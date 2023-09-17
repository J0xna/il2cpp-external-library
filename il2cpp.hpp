// include your own driver / memory library alongside all the dependencies from c++.

#define OFFSET( func, type, offset ) type func { return driver::read< type >( reinterpret_cast< std::uintptr_t >( this ) + offset ); }

namespace glb {
  inline std::uintptr_t game_assembly = 0;
	constexpr auto class_table_1 = 0x34AB9B8;
	constexpr auto class_table_2 = 0x34ABA78;
  	constexpr auto class_table_3 = 0x34AB980;
	constexpr auto assemblies_begin = 0x34AB6A0;
	constexpr auto assemblies_end = assemblies_begin + sizeof( void* );
}

struct il2cpp_field_t {
    OFFSET( offset( ), std::uint32_t, 0x18 );

    std::string name( ) {
        return driver::read_string( driver::read< std::uintptr_t >( this ), 64 );
    }
};

struct il2cpp_klass_t {
    OFFSET( name_ptr( ), std::uintptr_t, 0x10 );
    OFFSET( namespace_ptr( ), std::uintptr_t, 0x18 );
    OFFSET( fields_table( ), std::uintptr_t, 0x80 );
    OFFSET( static_fields_table( ), std::uintptr_t, 0xB8 );
    OFFSET( fields_size( ), std::uint16_t, 0x120 );

    il2cpp_field_t* get_field( const char* name ) {
        auto table = this->fields_table( );
        if ( !table ) {
            return nullptr;
        }

        for ( auto current_field = table; current_field < table + ( this->fields_size( ) * 0x20 ); current_field += 0x20 ) {
            auto field_name = driver::read_string( driver::read< std::uintptr_t >( current_field ), 64 );
            if ( !_stricmp( field_name.c_str( ), name ) ) {
                return reinterpret_cast< il2cpp_field_t* >( current_field );
            }
        }
        return nullptr;
    }

    template< typename t = std::uintptr_t >
    t get_static_field( const char* name ) {
        auto field = this->get_field( name );
        if ( !field ) {
            return 0;
        }

        return driver::read< t >( this->static_fields_table( ) + field->offset( ) );
    }

    std::string name( ) {
        return driver::read_string< std::uintptr_t >( this->name_ptr( ), 64 );
    }

    std::string ns( ) {
        return driver::read_string< std::uintptr_t >( this->namespace_ptr( ), 64 );
    }
};

struct il2cpp_image_t {
    il2cpp_klass_t* get_klass( const char* name, const char* ns = 0 ) {
        unsigned __int64 table_1 = driver::read< unsigned __int64 >( glb::game_assembly + glb::class_table_1 );    
        unsigned __int64 table_2 = driver::read< unsigned __int64 >( glb::game_assembly + glb::class_table_2 );
        unsigned __int64 table_3 = driver::read< unsigned __int64 >( glb::game_assembly + glb::class_table_3 );

        const auto class_idx = driver::read< std::uint32_t >( this + 0x18 );
        for ( int i = 0; i < class_idx; i++ ) { // broken, will not find all classes outside of Assembly-CSharp, try to fix this yourself if I don't fix it before (you can bruteforce them but not ideal)
            int v2 = i + driver::read< int >( driver::read< int >( this + 0x28 ) );
            if ( v2 == -1 )  {
                continue;
            }

            unsigned __int64 u1 = table_1 + driver::read< int >( table_2 + 0xA0 ) + 0x58 * v2;    
            unsigned __int64 index_handle = (u1 - driver::read< int >( table_2 + 0xA0 ) - table_1 ) / 0x58;
            unsigned __int64 u2 = ( unsigned __int64 )0x8 * ( int )index_handle;

            auto klass = driver::read< il2cpp_klass_t* >( u2 + table_3 );
            if ( !klass ) {
                continue;
            }

            auto klass_name = klass->name( );
            auto klass_ns = klass->ns( );

            if ( ns != 0 ) {
                if ( !_stricmp( klass_ns.c_str( ), ns ) && !_stricmp( klass_name.c_str( ), name ) ) {
                    return reinterpret_cast< il2cpp_klass_t* >( klass );
                }
            }
            else {
                if ( !_stricmp( klass_name.c_str( ), name ) ) {
                    return reinterpret_cast< il2cpp_klass_t* >( klass );
                }
            }
        }
        return nullptr;
    }
};

struct il2cpp {
    static inline std::vector< std::pair< std::string, il2cpp_image_t* > > images{ };

    static void initialize( ) {
        glb::game_assembly = driver::get_module< std::uintptr_t >( L"GameAssembly.dll" );
        if ( !glb::game_assembly ) {
            return;
        }

        const auto assemblies_end = driver::read< std::uintptr_t >( glb::game_assembly + offsets::assemblies_end );
        for ( auto current_assembly = driver::read< std::uintptr_t >( glb::game_assembly + offsets::assemblies_begin ); current_assembly < assemblies_end; current_assembly += sizeof( std::uintptr_t ) ) {
            images.emplace_back( driver::read_string( driver::read< std::uintptr_t >( driver::read< std::uintptr_t >( current_assembly ) + 0x18 ), 64 ), driver::read< il2cpp_image_t* >( driver::read< std::uintptr_t >( current_assembly ) ) );
        }
    }

    template< typename t = il2cpp_klass_t* >
    static t find_klass( const char* assembly_name, const char* klass_name, const char* ns = 0 ) {
        for ( int i = 0; i < images.size( ); i++ ) {
            const auto image = &images[ i ];
            if ( !_stricmp( image->first.c_str( ), assembly_name ) ) {
                const auto klass = image->second->get_klass( klass_name, ns );
                if ( !klass ) {
                    continue;
                }
                return ( t )klass;
            }
        }
        return nullptr;
    }

    static std::uintptr_t get_field_offset( const char* assembly, const char* klass_name, const char* field_name, const char* ns = 0 ) {
        const auto klass = find_klass( assembly, klass_name, ns );
        if ( !klass ) {
            return 0;
        }

        const auto field = klass->get_field( field_name );
        if ( !field ) {
            return 0;
        }

        return field->offset( );
    }

    static std::uintptr_t get_static_field_offset( const char* assembly, const char* klass_name, const char* field_name, const char* ns = 0 ) {
        const auto klass = find_klass( assembly, klass_name, ns );
        if ( !klass ) {
            return 0;
        }

        return klass->get_static_field( field_name );
    }
};
