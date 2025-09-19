#include <rz/idx.h>
#include <cmds/idx.h>

//template<typename Function, typename... Args>
//void callBy(Function func, Args... args) {
//	char* argv[] = {const_cast<char*>(args)..., nullptr}; // 添加 nullptr 作為結束標誌
//	int argc = sizeof...(args);
//	func(argc, argv);
//}

//#define callBy(func, ...) simulateMain(#func, func, __VA_ARGS__) //這樣不支持沒參數
#define callBy( func, ... ) simulateMain( #func, func, ##__VA_ARGS__ )


template<typename Function, typename... Args>
void simulateMain( const char *funcName, Function func, Args... args )
{
	std::string file = (std::filesystem::path( __FILE__ ).parent_path() / funcName).string();
	char *file_c = new char[file.size() + 1];
	std::strcpy( file_c, file.c_str() );
	std::vector<char *> argv = { file_c, (new char[std::strlen( args ) + 1], std::strcpy( new char[std::strlen( args ) + 1], args ))..., nullptr };
	func( argv.size() - 1, argv.data() );
	for ( char *ptr: argv ) delete[] ptr;
}





using namespace std;

int testAppSub( int argc, char *argv[] )
{
	cli::Ap ap( "args" );
//	ap.argHelp( "help,-h,--help" );

	ap.arg( "-v,--verb", "Print verbose during operation", [&]() { cli::print("set verb!"); } );
	ap.arg( "-ex", "Throw Error", [&]() { throw std::runtime_error( "!!-ex!!" ); } );

	//auto name = ap.Name();

	ap.cmd( "test", "test doc" )
	.handle( []( const cli::Args &args ) { cli::print( "test doc!!??" ); } );

	cli::Cmd& cmdConf = ap.cmd( "config", "The configs util" );

	cmdConf.cmd( "create", "Create conf file" )
	.handle( []( const cli::Args &args )
	{
		cli::print( "Creating conf file...\n" );
	} );
	cmdConf.cmd( "valid", "Validate conf file" )
	.handle( []( const cli::Args &args )
	{
		cli::print( "Validating conf file...\n" );
	} );

	ap
	.cmd( "sub", "Subtract two nums" )
	.arg<int>( "x" ).arg<int>( "y" )
	.handle( []( const cli::Args &args )
	{
		int x = args.get<int>( "x" );
		int y = args.get<int>( "y" );

		cli::print( cli::fmtstr("cal: x[{}] - y[{}] = [{}]\n", to_string(x), to_string(y), to_string(x - y)) );
	} );

	return ap.run( argc, argv );
}







int t_ap_noh( int argc, char *argv[] )
{
	cli::Ap ap( "single" );
	return ap.run( argc, argv );
}
int t_ap_bse( int argc, char *argv[] )
{
	cli::Ap ap( "single" );

	ap.handle([]( const cli::Args &args )
	{
		lg::info( "run ok! get length[{}]", args.length() );
	} );

	return ap.run( argc, argv );
}
int t_ap_sum( int argc, char *argv[] )
{
	cli::Ap ap( "single-sum" );

	ap.arg<int>( "x" ).arg<double>( "y" );
	//ap.arg( "", "", [](){} );

	ap.handle([]( const cli::Args &args )
	{
		lg::info( "run ok! get length[{}]", args.length() );
		//lg::info( "args: {}", args.toStr() );

		auto y = args.get<double>( "y" );
		auto x = args.get<int>( "x" );

		lg::info( "sum[ {} ]", x+y );
	} );

	return ap.run( argc, argv );
}


int main( int argc, char *argv[] )
{
	//printf("args：argc[%i]\n", ( argc ) );
	//printf("args：0[%s] 1[%s] 2[%s]\n", argv[0], argv[1], argv[2] );



	try
	{

		callBy( testAppSub, "sub", "100", "38" );

		//callBy( testAppSub, "args", "sub", "aaa123", "65" );
		//callBy( testAppSub, "args", "what", "100", "38" );
		//callBy( testAppSub, "args", "-v", "-v", "what", "100", "38" );
		//callBy( testAppSub, "args", "-v", "-v", "sub", "100", "38" );

		//callBy( t_ap_noh, "a", "b", "-v" );
		//callBy( t_ap_bse, "a", "b", "-v" );
		//callBy( t_ap_sum, "10", "9", "-v" );
	}
	catch ( std::exception &ex )
	{
		printf( "MainError: %s", ex.what() );
	}

	return 0;
}
