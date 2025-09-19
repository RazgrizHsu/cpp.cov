#include <chrono>
#include <string>

#include <iostream>
#include <fstream>
#include <vector>

#include "../_utils.hpp"
#include "rz/idx.h"

TEST_CASE( "std: Try_STD_Times" )
{

//	char date_buf[82];
//	auto now = system_clock::now();
//	time_t t = system_clock::to_time_t( now );
//	auto tm_time = rz::os::localtime( t );
//	std::strftime( date_buf, sizeof( date_buf ), "%Y-%m-%d %H:%M:%S.%ms", &tm_time );
//	lg::info( "now: [{}]", date_buf );

	using namespace rz::dt;

	auto sdt = "2020-01-02T03:04:05.678Z";

	auto allMs = strToUnix( sdt );
	REQUIRE_EQ( 1577934245678, allMs );
	//lg::info( "unix: {}", allMs );

	REQUIRE_EQ( 1577934245678, strToUnix( "2020-01-02T03:04:05.678" ) );
	REQUIRE_EQ( 1577934245678, strToUnix( "2020-01-02T03:04:05.678Z" ) );

	auto tp = strToTP( sdt );
	auto rst = tpToString( tp );
	//lg::info( "time[{}] count[{}]", rst, tp.time_since_epoch().count() );

	REQUIRE_EQ( rst, "2020-01-02T03:04:05.678" );


	//------------------------------------------------------------------------
	// truncate to minutes
	//------------------------------------------------------------------------
	const auto tpm = floor<std::chrono::minutes>( tp );
	auto rstm = tpToString( tpm );
	auto tpmc = strToUnix( rstm );
	//lg::info( "time[{}] count[{}] unix[{}]", rstm, tpm.time_since_epoch().count(), tpmc );

	REQUIRE_EQ( 1577934240000, tpmc );


	//------------------------------------------------------------------------
	// truncate to seconds
	//------------------------------------------------------------------------
	const auto tps = floor<std::chrono::seconds>( tp );
	auto rsts = tpToString( tps );
	auto rstc = strToUnix( rsts );
	//lg::info( "time[{}] count[{}] unix[{}]", rsts, tps.time_since_epoch().count(), rstc );

	REQUIRE_EQ( 1577934245000, rstc );



	//------------------------------------------------------------------------
	// 從unix轉回來
	//------------------------------------------------------------------------
	auto duration = 1577934245000;
	auto backTP = unixToTP( duration );
    auto dstr = tpToString( backTP );

	REQUIRE_EQ( "2020-01-02T03:04:05.000", dstr );

	//lg::info( "dst[{}]", dstr );


	auto utc = 1622766661000;

	lg::info( "true[{}] false[{}]", unixToStr( utc, true ), unixToStr( utc ) );
}


TEST_CASE( "std: vec_acc" )
{
	using namespace std;

	std::vector<int> vec = { 1, 2, 3 };

    auto rst = std::accumulate( vec.begin(), vec.end(), 0.0, [&]( auto a, auto b )
	{
    	return a + b;
	});

    REQUIRE_EQ( 6, rst );



    auto rst2 = std::accumulate( vec.begin(), vec.end(), 0, [&]( auto a, auto b )
	{
    	return a + b;
	}) / vec.size();


    REQUIRE_EQ( 2, rst2 );
}
