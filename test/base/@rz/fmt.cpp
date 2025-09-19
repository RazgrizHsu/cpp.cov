#include "doctest.h"
#include <regex>

#include "../_utils.hpp"
#include "rz/idx.h"

TEST_CASE( "str: STR_Base" )
{
	auto n1 = 0.12345678;

	REQUIRE_EQ( rz::fmt( "{:.2f}", n1 ), "0.12" );
	REQUIRE_EQ( rz::fmt( "{:.3f}", n1 ), "0.123" );
	REQUIRE_EQ( rz::fmt( "{:.9f}", n1 ), "0.123456780" );


	REQUIRE_EQ( rz::fmt( "{:>10}", "abc" ), "       abc" );

	//REQUIRE_EQ( rz::str::pad( "test", 6 ), "  test" );
}

TEST_CASE( "str: STR_ERR" )
{
	double s1 = 12345;

	lg::info( "dump[{:0.2f}]", s1 );


	auto byteS = 12345;

	lg::info( "rst[{}]", rz::fmt( "{:0.2f}MB", ( byteS / 1024 / 1024.L ) ) );
	lg::info( "rst[{}]", rz::fmt( "{:0.ff}MB", ( byteS / 1024.L / 1024.L ) ) );

	//lg::info( "rst: [{}]", fmt::format( "{:0.ff}MB", ( byteS / 1024.L / 1024.L ) ) );

	auto doub = rz::fmt( "{:10.2f}", 12345.67890L );

	REQUIRE_EQ( doub, "  12345.68" );
}




TEST_CASE( "str: STR_RegEx" )
{
	auto s = "abc12za56";
	std::regex e(R"(\d+)");

	auto rst = std::regex_replace(s, e, "XX");

	REQUIRE_EQ( rst, "abcXXzaXX" );
}
