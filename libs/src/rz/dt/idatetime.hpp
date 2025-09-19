#pragma once

#include <chrono>
#include <string>
#include <iomanip>
#include <sstream>
#include <regex>
#include <optional>
#include <ctime>


namespace rz::dt {
namespace cho = std::chrono;

static void replaceAll( std::string &str, const std::string &from, const std::string &to ) {
	size_t start_pos = 0;
	while ( ( start_pos = str.find( from, start_pos ) ) != std::string::npos ) {
		str.replace( start_pos, from.length(), to );
		start_pos += to.length();
	}
}

static std::string padZero( long long value, int width ) {
	std::ostringstream oss;
	oss << std::setw( width ) << std::setfill( '0' ) << value;
	return oss.str();
}

class IDU {
	cho::milliseconds duration;

public:
	IDU() : duration( 0 ) {}
	explicit IDU( long long ms ) : duration( ms ) {}
	explicit IDU( const std::string &durationStr ) {
		*this = parse( durationStr );
	}

	long long ms() const { return duration.count(); }
	double secs() const { return duration.count() / 1000.0; }
	double mins() const { return duration.count() / 60000.0; }
	double hours() const { return duration.count() / 3600000.0; }
	double days() const { return duration.count() / 86400000.0; }

	IDU operator+( const IDU &other ) const { return IDU( duration.count() + other.duration.count() ); }
	IDU operator-( const IDU &other ) const { return IDU( duration.count() - other.duration.count() ); }

	static IDU secs( double seconds ) { return IDU( static_cast<long long>(seconds * 1000) ); }
	static IDU mins( double minutes ) { return IDU( static_cast<long long>(minutes * 60 * 1000) ); }
	static IDU hours( double hours ) { return IDU( static_cast<long long>(hours * 3600 * 1000) ); }
	static IDU days( double days ) { return IDU( static_cast<long long>(days * 24 * 3600 * 1000) ); }

	IDU &operator+=( const IDU &other ) {
		duration += other.duration;
		return *this;
	}
	IDU &operator-=( const IDU &other ) {
		duration -= other.duration;
		return *this;
	}
	IDU operator*( double factor ) const { return IDU( static_cast<long long>(duration.count() * factor) ); }
	IDU operator/( double divisor ) const { return IDU( static_cast<long long>(duration.count() / divisor) ); }

	bool operator==( const IDU &other ) const { return duration == other.duration; }
	bool operator!=( const IDU &other ) const { return duration != other.duration; }
	bool operator<( const IDU &other ) const { return duration < other.duration; }
	bool operator>( const IDU &other ) const { return duration > other.duration; }
	bool operator<=( const IDU &other ) const { return duration <= other.duration; }
	bool operator>=( const IDU &other ) const { return duration >= other.duration; }

	std::string human( bool includeMs = false ) const {
		long long totalMs = duration.count();
		long long days = totalMs / ( 24 * 3600 * 1000 );
		totalMs %= ( 24 * 3600 * 1000 );
		long long hours = totalMs / ( 3600 * 1000 );
		totalMs %= ( 3600 * 1000 );
		long long minutes = totalMs / ( 60 * 1000 );
		totalMs %= ( 60 * 1000 );
		long long seconds = totalMs / 1000;
		long long ms = totalMs % 1000;

		std::ostringstream oss;
		if ( days > 0 ) oss << days << "d ";
		if ( hours > 0 ) oss << hours << "h ";
		if ( minutes > 0 ) oss << minutes << "m ";

		if ( includeMs ) {
			oss << std::fixed << std::setprecision( 3 ) << ( seconds + ms / 1000.0 ) << "s";
		}
		else {
			oss << seconds << "s";
		}

		return oss.str();
	}

	static IDU parse( const std::string &durationStr ) {
		long long totalMs = 0;
		std::regex rxIso8601( R"(P(?:([0-9]+)D)?(?:T(?:([0-9]+)H)?(?:([0-9]+)M)?(?:([0-9]+(?:\.[0-9]+)?)S)?)?)" );
		std::regex rxTime( R"(([0-9]{2}):([0-9]{2}):([0-9]{2})(?:\.([0-9]{1,3}))?)" );
		std::regex rxSecs( R"(([0-9]+(?:\.[0-9]+)?))" );
		std::smatch matches;

		if ( std::regex_match( durationStr, matches, rxIso8601 ) ) {
			if ( matches[1].matched ) totalMs += std::stoll( matches[1] ) * 24 * 3600 * 1000;
			if ( matches[2].matched ) totalMs += std::stoll( matches[2] ) * 3600 * 1000;
			if ( matches[3].matched ) totalMs += std::stoll( matches[3] ) * 60 * 1000;
			if ( matches[4].matched ) totalMs += static_cast<long long>(std::stod( matches[4] ) * 1000);
		}
		else if ( std::regex_match( durationStr, matches, rxTime ) ) {
			totalMs += std::stoll( matches[1] ) * 3600 * 1000; // hours
			totalMs += std::stoll( matches[2] ) * 60 * 1000;   // minutes
			totalMs += std::stoll( matches[3] ) * 1000;        // seconds
			if ( matches[4].matched ) {
				std::string msStr = matches[4].str() + std::string( 3 - matches[4].length(), '0' );
				totalMs += std::stoll( msStr ); // milliseconds
			}
		}
		else if ( std::regex_match( durationStr, matches, rxSecs ) ) {
			double seconds = std::stod( matches[1] );
			totalMs = static_cast<long long>(seconds * 1000);
		}
		else {
			throw std::invalid_argument( "Invalid duration format value[" + durationStr + "]" );
		}
		return IDU( totalMs );
	}

	std::string iso8601() const {
		long long totalSeconds = duration.count() / 1000;
		long long days = totalSeconds / 86400;
		long long hours = ( totalSeconds % 86400 ) / 3600;
		long long minutes = ( totalSeconds % 3600 ) / 60;
		double seconds = ( duration.count() % 60000 ) / 1000.0;

		std::stringstream ss;
		ss << 'P';
		if ( days > 0 ) ss << days << 'D';
		if ( hours > 0 || minutes > 0 || seconds > 0 ) {
			ss << 'T';
			if ( hours > 0 ) ss << hours << 'H';
			if ( minutes > 0 ) ss << minutes << 'M';
			if ( seconds > 0 ) ss << std::fixed << std::setprecision( 3 ) << seconds << 'S';
		}
		return ss.str();
	}


	std::string str( const std::string &format = "" ) const {
		if ( format.empty() ) return iso8601();

		long long totalMilliseconds = duration.count();
		long long hours = totalMilliseconds / 3600000;
		long long minutes = ( totalMilliseconds % 3600000 ) / 60000;
		long long seconds = ( totalMilliseconds % 60000 ) / 1000;
		long long milliseconds = totalMilliseconds % 1000;

		std::string result = format;
		replaceAll( result, "HH", padZero( hours, 2 ) );
		replaceAll( result, "H", std::to_string( hours ) );
		replaceAll( result, "mm", padZero( minutes, 2 ) );
		replaceAll( result, "m", std::to_string( minutes ) );
		replaceAll( result, "ss", padZero( seconds, 2 ) );
		replaceAll( result, "s", std::to_string( seconds ) );

		if ( result.find( "SSS" ) != std::string::npos ) replaceAll( result, "SSS", padZero( milliseconds, 3 ) );
		if ( result.find( "SS" ) != std::string::npos ) replaceAll( result, "SS", padZero( milliseconds / 10, 2 ) );
		if ( result.find( "S" ) != std::string::npos ) replaceAll( result, "S", padZero( milliseconds / 100, 1 ) );


		return result;
	}

	std::string strSecs() const { return str( "HH:mm:ss" ); }
	std::string strfull() const { return str( "HH:mm:ss.SSS" ); }


};

/*
// - 各屬性都是顯示當地時間
//
//
*/
class IDT {
	std::tm td;
	int ms;
	int zmins;

	void patchTZ( int new_zmins ) {
		int diff = new_zmins - zmins;
		std::time_t t = timegm( &td ) + diff * 60;
		td = *std::gmtime( &t );
		zmins = new_zmins;
		mktime( &td );
	}

	static std::optional<IDT> parseImpl( const std::string &dateTimeStr ) {
		std::regex iso8601_regex(
			R"(^(\d{4})-(\d{2})-(\d{2})(?:[T ](\d{2}):(\d{2}):(\d{2})(?:\.(\d{1,3}))?)?(?:Z|([+-])(\d{2}):?(\d{2})?)?$)"
		);
		std::smatch match;

		if ( !std::regex_match( dateTimeStr, match, iso8601_regex ) ) {
			return std::nullopt;
		}

		IDT result;
		std::tm &tm = result.td;

		tm.tm_year = std::stoi( match[1] ) - 1900;
		tm.tm_mon = std::stoi( match[2] ) - 1;
		tm.tm_mday = std::stoi( match[3] );

		if ( match[4].matched ) { // if time part exists
			tm.tm_hour = std::stoi( match[4] );
			tm.tm_min = std::stoi( match[5] );
			tm.tm_sec = std::stoi( match[6] );
		}
		else {
			tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
		}

		auto ms = match[7].matched ? std::stoi( match[7] ) : 0;
		while ( ms > 0 && ms < 100 && match[7].matched ) ms *= 10; // ensure milliseconds is three digits
		result.ms = ms;

		if ( match[8].matched ) { // if timezone info exists
			int tz_hour = std::stoi( match[9] );
			int tz_min = match[10].matched ? std::stoi( match[10] ) : 0;
			result.zmins = ( tz_hour * 60 + tz_min ) * ( match[8] == "+" ? 1 : -1 );
		}

		mktime( &result.td );
		return result;
	}

public:
	IDT() {
		auto now = std::chrono::system_clock::now();
		auto now_c = std::chrono::system_clock::to_time_t( now );

		std::tm local_tm = *std::localtime( &now_c );
		std::tm utc_tm = *std::gmtime( &now_c );

		td = local_tm; // 使用本地時間
		ms = std::chrono::duration_cast<std::chrono::milliseconds>( now.time_since_epoch() ).count() % 1000;

		// 計算本地時間和 UTC 時間之間的差異
		std::time_t local_tt = std::mktime( &local_tm );
		std::time_t utc_tt = std::mktime( &utc_tm );

		// 計算差異（秒），然後轉換為分鐘
		long diff_seconds = std::difftime( local_tt, utc_tt );
		zmins = static_cast<int>(diff_seconds / 60);
	}

	IDT( long long unix_ms, const std::optional<std::string> &t = std::nullopt ) {
		time_t seconds = unix_ms / 1000;
		td = *std::gmtime( &seconds );
		ms = unix_ms % 1000;
		zmins = 0;
		if ( t ) tz( *t );
	}

	IDT( const std::string &iso8601_string ) {
		auto parsed = parseImpl( iso8601_string );
		if ( !parsed ) {
			throw std::invalid_argument( "Invalid ISO 8601 format" );
		}
		*this = *parsed;
	}

	static std::optional<IDT> parse( const std::string &dateTimeStr ) {
		return parseImpl( dateTimeStr );
	}


	int tz( const std::optional<std::string> &tz = std::nullopt ) {
		if ( tz ) {
			const std::regex tz_regex( "([+-])(\\d{1,2})(?::(\\d{2}))?" );
			if ( std::smatch mhs; std::regex_match( *tz, mhs, tz_regex ) ) {
				int hs = std::stoi( mhs[2] );
				int mins = mhs[3].matched ? std::stoi( mhs[3] ) : 0;
				int new_zmins = ( hs * 60 + mins ) * ( mhs[1] == "-" ? -1 : 1 );
				patchTZ( new_zmins );
			}
			else {
				patchTZ( 0 ); // 如果格式無效，假設為 UTC
			}
		}
		return zmins;
	}


	long long unixtime() const {
		std::tm temp = td;
		temp.tm_sec -= zmins * 60; // 調整為 UTC 時間
		return timegm( &temp ) * 1000LL + ms;
	}

	std::string strftime( const std::string &fmt = "%Y-%m-%d %H:%M:%S" ) const {
		char buffer[100];
		std::strftime( buffer, sizeof( buffer ), fmt.c_str(), &td );
		return std::string( buffer );
	}

	//ISO8601-基於當前時間的
	std::string iso() const {

		std::tm tm = td;

		std::stringstream ss;
		ss << std::put_time( &tm, "%Y-%m-%dT%H:%M:%S" )
			<< "." << std::setfill( '0' ) << std::setw( 3 ) << ms;

		if ( zmins == 0 ) ss << "Z";
		else {
			ss << ( zmins > 0 ? "+" : "-" )
				<< std::setfill( '0' ) << std::setw( 2 ) << std::abs( zmins ) / 60
				<< ":" << std::setw( 2 ) << std::abs( zmins ) % 60;
		}

		return ss.str();
	}

	//基於utc的時間，不帶時區
	std::string utc( const std::string &format = "YYYY-MM-DD HH:mm:ss.SSS" ) const {

		if ( format.find( "Z" ) != std::string::npos ) throw std::invalid_argument( "should not have Z" );

		std::tm tm = td;
		std::time_t t = timegm( &tm ); // 轉換為 UTC 時間戳
		t -= zmins * 60;               // 應用時區偏移
		tm = *gmtime( &t );

		std::string result = format;
		replaceAll( result, "YYYY", std::to_string( tm.tm_year + 1900 ) );
		replaceAll( result, "MM", padZero( tm.tm_mon + 1, 2 ) );
		replaceAll( result, "DD", padZero( tm.tm_mday, 2 ) );
		replaceAll( result, "HH", padZero( tm.tm_hour, 2 ) );
		replaceAll( result, "mm", padZero( tm.tm_min, 2 ) );
		replaceAll( result, "ss", padZero( tm.tm_sec, 2 ) );
		replaceAll( result, "SSS", padZero( ms, 3 ) );

		return result;
	}

	//當地時間
	std::string str( const std::string &format = "YYYY-MM-DD HH:mm:ss.SSS" ) const {

		if ( format.find( "Z" ) != std::string::npos ) throw std::invalid_argument( "should not have Z" );
		std::tm tm = td;
		std::string result = format;
		replaceAll( result, "YYYY", std::to_string( tm.tm_year + 1900 ) );
		replaceAll( result, "MM", padZero( tm.tm_mon + 1, 2 ) );
		replaceAll( result, "DD", padZero( tm.tm_mday, 2 ) );
		replaceAll( result, "HH", padZero( tm.tm_hour, 2 ) );
		replaceAll( result, "mm", padZero( tm.tm_min, 2 ) );
		replaceAll( result, "ss", padZero( tm.tm_sec, 2 ) );
		replaceAll( result, "SSS", padZero( ms, 3 ) );

		return result;
	}


	std::string zoneStr() const {
		std::stringstream ss;
		ss << "UTC";
		if ( zmins != 0 ) {
			int hs = std::abs( zmins ) / 60;
			int mins = std::abs( zmins ) % 60;
			ss << ( zmins > 0 ? "+" : "-" ) << std::setfill( '0' ) << std::setw( 2 ) << hs;
			ss << ":" << std::setw( 2 ) << mins;
		}
		return ss.str();
	}


	int dayOfWeek() const { return td.tm_wday; }
	int dayOfYear() const { return td.tm_yday + 1; }
	bool isLeapYear() {
		int y = year();
		return ( y % 4 == 0 && y % 100 != 0 ) || ( y % 400 == 0 );
	}

	int year( std::optional<int> y = std::nullopt ) {
		if ( y ) td.tm_year = *y - 1900;
		return td.tm_year + 1900;
	}
	int month( std::optional<int> m = std::nullopt ) {
		if ( m ) td.tm_mon = *m - 1;
		return td.tm_mon + 1;
	}
	int day( std::optional<int> d = std::nullopt ) {
		if ( d ) td.tm_mday = *d;
		return td.tm_mday;
	}
	int hour( std::optional<int> h = std::nullopt ) {
		if ( h ) td.tm_hour = *h;
		return td.tm_hour;
	}
	int minute( std::optional<int> m = std::nullopt ) {
		if ( m ) td.tm_min = *m;
		return td.tm_min;
	}
	int second( std::optional<int> s = std::nullopt ) {
		if ( s ) td.tm_sec = *s;
		return td.tm_sec;
	}
	int millisecond( std::optional<int> v = std::nullopt ) {
		if ( v ) ms = *v;
		return ms;
	}

	bool operator==( const IDT &other ) const { return unixtime() == other.unixtime() && zmins == other.zmins; }
	bool operator!=( const IDT &other ) const { return !( *this == other ); }
	bool operator<( const IDT &other ) const { return unixtime() < other.unixtime(); }
	bool operator>( const IDT &other ) const { return other < *this; }
	bool operator<=( const IDT &other ) const { return !( other < *this ); }
	bool operator>=( const IDT &other ) const { return !( *this < other ); }

	IDT &operator+=( const IDU &duration ) {
		long long secs = duration.ms() / 1000;
		int nms = duration.ms() % 1000;

		ms += nms;
		if ( ms >= 1000 ) {
			secs += ms / 1000;
			ms %= 1000;
		}

		td.tm_sec += secs;
		std::mktime( &td );

		return *this;
	}
	IDT &operator-=( const IDU &duration ) { return *this += IDU( -duration.ms() ); }

	IDT operator+( const IDU &du ) const { return IDT( unixtime() + du.ms(), zoneStr() ); }
	IDT operator-( const IDU &du ) const { return IDT( unixtime() - du.ms(), zoneStr() ); }
	IDU operator-( const IDT &other ) const { return IDU( unixtime() - other.unixtime() ); }



};



}
