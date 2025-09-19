#pragma once

#include "./date/date.hpp"
#include "./idatetime.hpp"

#define co std::chrono
#define TPms date::local_time<std::chrono::milliseconds>

namespace rz::dt {
namespace cho = std::chrono;

TPms strToTP( std::string const &str, std::string const &format = "%FT%T" );
std::string tpToString( TPms tp, std::string const &format = "%Y-%m-%dT%H:%M:%S" );
long strToUnix( std::string const &str );
TPms unixToTP( long unixtime );
long unixTruncateToSeconds( long unixtime );
std::string unixToStr( long unixtime, bool truncateSecond = false );

std::string nowTimeStr();



// 到ms
inline long now( int timezome = 0 ) {

	auto now = cho::system_clock::now(); // UTC+0
	auto unix_timestamp = cho::duration_cast<cho::milliseconds>( now.time_since_epoch() ).count();

	//std::cout << "UTC+0 Unix時間戳: " << unix_timestamp << std::endl;

	if ( timezome != 0 ) unix_timestamp += ( timezome * 3600 ); // 加上timezone的秒數

	return unix_timestamp;
}

}

#undef co
#undef TPms
