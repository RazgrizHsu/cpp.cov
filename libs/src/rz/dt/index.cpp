
#include "./idx.h"

#define co std::chrono
#define TPms date::local_time<std::chrono::milliseconds>

namespace rz::dt {


//using namespace std::chrono;


TPms strToTP( std::string const &str, std::string const &format )
{
	date::local_time<co::milliseconds> tp;
	std::istringstream strS( str );
	strS >> date::parse( format, tp );
	return tp;
}

std::string tpToString( TPms tp, std::string const &format )
{
	try
	{
		auto s = date::format( format, tp );
		return s;
	}
	catch( ... )
	{
		return date::format( "%FT%T", tp );
	}
}

long strToUnix( std::string const &str )
{
	auto tp = strToTP( str );

	long rst = tp.time_since_epoch().count();
	//if( rst == 0 ) lg::warn( "[strToUnix] 轉換出來為0, str[{}]", str );

	return rst;
}

TPms unixToTP( long unixtime )
{
	// ?auto backTP = co::system_clock::from_time_t( utc );

	co::milliseconds dur(unixtime);
	co::time_point<co::local_time<co::milliseconds>> dt(dur);

	date::local_time<co::milliseconds> lts( dur );

	return lts;
}

// 從unix截到秒
long unixTruncateToSeconds( long unixtime )
{
	auto tp = unixToTP( unixtime );

	auto tpm = floor<co::seconds>( tp );
	auto tps = tpToString( tpm );
	return strToUnix( tps );
}

std::string unixToStr( long unixtime, bool truncateSecond )
{
	auto tp = unixToTP( unixtime );

	auto str = tpToString( tp );

	if( truncateSecond )
	{
		auto idx = str.find( ".", 0 );
		if( idx != std::string::npos ) str.replace( idx, 4, "" );
	}
	return str;
}

std::string nowTimeStr()
{
	auto now = std::chrono::system_clock::now();
	std::time_t now_time = std::chrono::system_clock::to_time_t( now );
	char buffer[20];
	std::strftime( buffer, sizeof( buffer ), "%Y%m%d%H%M%S", std::localtime( &now_time ) );
	return { buffer };
}




}