#include "doctest.h"
#include <chrono>
#include <string>

#include <iostream>
#include <fstream>
#include <vector>

#include "../_utils.hpp"
#include "rz/idx.h"

TEST_CASE( "str: replace" )
{
	std::string base = "abc123456efg";

	rz::str::replaceAll( base, "123", "@@@" );

	lg::info( "result: {}", base );
}


TEST_CASE( "str: detectLang" )
{
	auto s1 = "abc123456efg";
	auto s2 = "abc123かzzve";
	auto s3 = "abc測試中文字zzve";
	REQUIRE_EQ( rz::str::hasJP( s1 ), false );
	REQUIRE_EQ( rz::str::hasJP( s2 ), true );
	REQUIRE_EQ( rz::str::hasTW( s2 ), false );
	REQUIRE_EQ( rz::str::hasTW( s3 ), true );
}


TEST_CASE( "memstr: the_memstr" )
{
	rz::memstr::Store store;


	auto ref1 = store.Add( "a" );
	auto ref2 = store.Add( "a" );

	REQUIRE_EQ( ref1, ref2 );


	auto rst = store.ToString( ref1 );

	lg::info( "rst: {}", rst );
}


TEST_CASE( "str: match" )
{
	auto s1 = "(www.test.net) U-RL";
	auto m1 = rz::str::contains( s1, "test", true );
	REQUIRE_EQ( m1, true );

	vector<string> excludes{ "net", ".url", "core." };

	auto m2 = rz::str::contains( s1, excludes, true );
	REQUIRE_EQ( m2, true );
}
