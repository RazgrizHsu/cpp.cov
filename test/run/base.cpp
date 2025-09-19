#include "doctest.h"

#include <algorithm>
#include <deque>

#include <rz/idx.h>
#include <cmds/cov/idx.h>

using namespace rz;

using namespace rz::io;
using namespace rz::mda;

TEST_CASE( "base: rgx_rename" )
{

	vector<string> strs = {
		"[TEST] 【test】this is test [ZZ001]",
	};
	for ( const auto &str: strs ) {

		auto rst = rz::mda::vo::removeNoNeeds( {}, str );
		lg::info( "\n  src[{}]\n  rst[{}]", str, rst );
	}
}



TEST_CASE( "base: rgx_rename_pic" ) {

	vector<string> strs = {
		"/path/to/(ABC) test (DEF)",
	};
	for ( const auto &str: strs ) {

		auto rst = img::removeNoNeeds( str );
		auto rnof = rz::io::fil::rmext( rst );
		lg::info( "\n  src[{}]\n  rst[{}]\n  noext[{}]", str, rst, rnof );
	}

	lg::info( "done" );
}


TEST_CASE( "test: " ) {


	cmds::cov::doVideos( IDir::make( "/Volumes/raz/car/cam" ).get() );


}
