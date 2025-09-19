#include "doctest.h"


#include <algorithm>
#include <deque>
#include <sstream>

#include <rz/idx.h>

using namespace rz;



TEST_CASE( "net: readJson" ) {

	auto json = Json::parse( "{}" );
	// auto fil = io::fil::read( pjson );
	// auto json = Json::parse( fil );
	//
	// lg::info( "json: {}", json.dump(0) );
	// lg::info( "json: {}", json.dump(4) );
	//
	//
	// io::fil::save( "/Volumes/tmp/test2.json", json.dump(4) );
}

// TEST_CASE( "net: basic" ) {
// 	httplib::Client cli( "https://resp.api" );
//
// 	stringstream ss;
//
// 	auto res = cli.Get( "/api/test",
// 		[&]( const char *data, size_t data_length ) {
// 			ss.write( data, data_length );
// 			return true;
// 		},
// 		[]( uint64_t current, uint64_t total ) {
// 			lg::info( "Download progress: {} / {}", current, total );
// 			return true;
// 		}
// 	);
//
// 	if ( res ) {
//
// 		auto str = ss.str();
// 		//lg::info( "data(sstr): {}", str );
// 		auto json = Json::parse( str );
// 		//lg::info( "data(json): {}", json.dump(4) );
//
// 		io::fil::save( "/Volumes/tmp/test_download.json", json.dump( 4 ) );
// 	}
// 	else {
// 		std::cout << "\nDownload failed. Error: " << httplib::to_string( res.error() ) << std::endl;
// 	}
//
// }


