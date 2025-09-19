#include "doctest.h"
#include <cmds/cli.h>
#include <sstream>

// Helper function to run CLI and capture output
std::pair<int, std::string> runCliAndCaptureOutput( const cli::Ap &ap, const std::vector<const char *> &args ) {
	std::stringstream captured_output;

	// Save original printer and set custom one to capture output
	cli::IFnActOut original_printer = cli::pv::getPrinter();
	cli::setPrinter([&captured_output](const std::string& msg) {
		captured_output << msg << "\n";
	});

	cli::err::clear();
	int result = ap.run( args.size() - 1, const_cast<char **>(args.data()) );

	// Restore original printer
	cli::setPrinter(std::move(original_printer));

	std::string output = captured_output.str();
	if ( cli::err::has() ) output += "\nErrors:\n" + cli::err::allStr();
	return { result, output };
}

// Helper function to create a basic Ap instance
cli::Ap createBasicAp() {
	cli::Ap ap( "cov" );
	ap.flag( "--clean", "Clean flag" );
	ap.cmd( "sum", "Sum two numbers" )
	  .arg<int>( "num1" )
	  .arg<int>( "num2" )
	  .handle( []( const cli::Args &args ) {
		  int num1 = args.get<int>( "num1" );
		  int num2 = args.get<int>( "num2" );
		  cli::print( std::to_string( num1 + num2 ) );
		  return 0;
	  } );
	return ap;
}

TEST_CASE( "cli: total" ) {
	// Test 1: Path and flag
	{
		cli::Ap ap = createBasicAp();
		bool cleanFlag = false;
		ap.handle( [&]( const cli::Args &args ) {
			cleanFlag = args.get<bool>( "--clean" );
			return 0;
		} );

		auto [result, output] = runCliAndCaptureOutput( ap, { "cov", "/path/to", "--clean", nullptr } );

		CHECK_EQ( result, 0 );
		CHECK( cleanFlag );
		CHECK( output.empty() );
		CHECK_FALSE( cli::err::has() );
	}

	// Test 2: Sub command sum
	{
		cli::Ap ap = createBasicAp();
		auto [result, output] = runCliAndCaptureOutput( ap, { "cov", "sum", "10", "50", nullptr } );

		CHECK_EQ( result, 0 );
		CHECK_EQ( output, "60\n" );
		CHECK_FALSE( cli::err::has() );
	}

	// Test 3: Invalid input prints help
	{
		cli::Ap ap = createBasicAp();
		auto [result, output] = runCliAndCaptureOutput( ap, { "cov", "invalid_command", nullptr } );

		CHECK_NE( result, 0 );
		CHECK( output.find("Usage:") != std::string::npos );
		CHECK( output.find("Description:") != std::string::npos );
		CHECK( output.find("Args:") != std::string::npos );
		CHECK( output.find("Subs:") != std::string::npos );
		CHECK_FALSE( cli::err::has() );
	}

	// Test 4: Missing required argument
	{
		cli::Ap ap = createBasicAp();
		auto [result, output] = runCliAndCaptureOutput( ap, { "cov", "sum", "10", nullptr } );

		CHECK_NE( result, 0 );
		CHECK( output.find("Missing required argument") != std::string::npos );
		CHECK( cli::err::has() );
	}

	// Test 5: Optional argument
	{
		cli::Ap ap( "test" );
		ap.arg<int>( "required" )
		  .arg<std::string>( "optional", "", true )
		  .handle( []( const cli::Args &args ) {
			  int req = args.get<int>( "required" );
			  std::string opt = args.get<std::string>( "optional", "default" );
			  cli::print( std::to_string( req ) + " " + opt );
			  return 0;
		  } );

		{
			auto [result, output] = runCliAndCaptureOutput( ap, { "test", "42", "custom", nullptr } );
			CHECK_EQ( result, 0 );
			CHECK_EQ( output, "42 custom\n" );
			CHECK_FALSE( cli::err::has() );
		}

		{
			auto [result, output] = runCliAndCaptureOutput( ap, { "test", "42", nullptr } );
			CHECK_EQ( result, 0 );
			CHECK_EQ( output, "42 default\n" );
			CHECK_FALSE( cli::err::has() );
		}
	}

	// Test 6: Type mismatch
	{
		cli::Ap ap( "test" );
		ap.arg<int>( "number" )
		  .handle( []( const cli::Args &args ) {
			  int num = args.get<int>( "number" );
			  cli::print( std::to_string( num ) );
			  return 0;
		  } );

		auto [result, output] = runCliAndCaptureOutput( ap, { "test", "not_a_number", nullptr } );

		CHECK_NE( result, 0 );
		CHECK( output.find("Invalid argument (number) value[not_a_number] for type[int]") != std::string::npos );
		CHECK( cli::err::has() );
	}


	// Test 7: All optional arguments
	{
		cli::Ap ap( "test" );
		ap.arg<std::string>( "path", "convert path if not set use current", true )
		  .flag( "-clean", "clean all .cov files" )
		  .handle( []( const cli::Args &args ) {
			  auto path = args.get<std::string>( "path", "default_path" );
			  auto clean = args.get<bool>( "-clean", false );
			  cli::print( "Path: " + path + ", Clean: " + ( clean ? "true" : "false" ) );
			  return 0;
		  } );

		// Test with no arguments
		{
			cli::err::clear();
			auto [result, output] = runCliAndCaptureOutput( ap, { "test", nullptr } );
			INFO("Expected result 0 for no arguments");
			CHECK_EQ( result, 0 );
			INFO("Unexpected output for no arguments");
			CHECK_EQ( output, "Path: default_path, Clean: false\n" );
			INFO("Unexpected errors for no arguments");
			CHECK( cli::err::all().empty() );
		}

		// Test with optional argument
		{
			cli::err::clear();
			auto [result, output] = runCliAndCaptureOutput( ap, { "test", "custom_path", nullptr } );
			INFO("Expected result 0 for optional argument");
			CHECK_EQ( result, 0 );
			INFO("Unexpected output for optional argument");
			CHECK_EQ( output, "Path: custom_path, Clean: false\n" );
			INFO("Unexpected errors for optional argument");
			CHECK( cli::err::all().empty() );
		}

		// Test with flag
		{
			auto [result, output] = runCliAndCaptureOutput( ap, { "test", "-clean", nullptr } );
			INFO("Expected result 0 for flag");
			CHECK_EQ( result, 0 );
			INFO("Unexpected output for flag");
			CHECK_EQ( output, "Path: default_path, Clean: true\n" );
			INFO("Unexpected errors for flag");
			CHECK_FALSE( cli::err::has() );
		}

		// Test with both optional argument and flag
		{
			cli::err::clear();
			auto [result, output] = runCliAndCaptureOutput( ap, { "test", "custom_path", "-clean", nullptr } );
			INFO("Expected result 0 for optional argument and flag");
			CHECK_EQ( result, 0 );
			INFO("Unexpected output for optional argument and flag");
			CHECK_EQ( output, "Path: custom_path, Clean: true\n" );
			INFO("Unexpected errors for optional argument and flag");
			CHECK( cli::err::all().empty() );
		}
	}
}
