#include "doctest.h"

#include "rz/idx.h"

enum TRY {
	None = 0,
	A = 1, B = 2,
};


TEST_CASE( "enum: converts" )
{
	auto v = TRY::A;

	if( v ) {
		v = (TRY)2;
	}
	lg::info( "value[{}]", v );

	if( v == TRY::B ) lg::info( "OK!!" );
}


enum class TRY2 {
	None = 0,
	A = 1, B = 2,
};


TEST_CASE( "enum: converts2" )
{
	auto v = TRY2::A;

	if( (int)v ) {
		v = (TRY2)2;
	}
	lg::info( "value[{}]", v );

	if( v == TRY2::B ) lg::info( "OK!!" );
}
