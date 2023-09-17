# il2cpp-external-library

Example usage:
```
int main( ) {
	il2cpp::initialize( );

	auto base_player = il2cpp::find_klass( "Assembly-CSharp", "BasePlayer" );
	if ( base_player ) {
		auto eyes = base_player->get_field( "eyes" );
		printf( "%p\n", eyes );
	}
	return 0;
}
```
