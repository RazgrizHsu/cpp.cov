#pragma once

#include <functional>


namespace rz::cmp {

static bool ignoreCase( const string &str1, const string &str2 )
{
	auto it1 = str1.begin();
	auto it2 = str2.begin();

	while ( it1 != str1.end() && it2 != str2.end() )
	{
		if ( tolower( *it1 ) != tolower( *it2 ) ) return false;
		++ it1;
		++ it2;
	}

	return it1 == str1.end() && it2 == str2.end();
}


}