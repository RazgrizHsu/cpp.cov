#include "doctest.h"
#include <numeric>

#include "rz/idx.h"

TEST_CASE( "math: base" )
{
	long nl1 = 1000;

	auto rst1 = nl1 / 3;

	lg::info( "rst: [{}]", rst1 );
	lg::info( "rst: [{}]", typeid(rst1).name() );

	auto rst2 = nl1 / 3.L;

	lg::info( "rst: [{}]", rst2 );
	lg::info( "rst: [{}]", typeid(rst2).name() );



	REQUIRE( rz::equal( 0.95,0.948, 1e-2 ) );
	REQUIRE( rz::equal( 0.95,0.948, 1e-3 ) == false );
	REQUIRE( rz::equal( 0.95,0.9505 ) );
}
