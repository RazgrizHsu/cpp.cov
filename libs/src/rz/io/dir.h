#pragma once
#include <filesystem>
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>

namespace rz::io::dir {

namespace fs = std::filesystem;

//例如 /a = 0 /a/b = 1
inline size_t depth( const fs::path &path ) {
	size_t depth = 0;
	for ( const auto &_: path ) ++depth;
	return depth - 1;
}

}



namespace rz::io::dir {


inline fs::path ensureNew( const fs::path &dir ) {
	if ( !fs::exists( dir ) ) return dir;

	fs::path base = dir;
	std::string name = base.filename().string();
	base.remove_filename();

	int c = 1;
	fs::path pnew;
	do {
		pnew = base / ( name + " (" + std::to_string( c ) + ")" );
		c++;
	}
	while ( fs::exists( pnew ) );

	return pnew;
}

inline std::string ensureNewName( const fs::path &dir ) {
	auto pnew = ensureNew( dir );
	return pnew.filename().string();
}


//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
inline std::string maketmpId() {
	// 使用當前時間作為種子
	auto now = std::chrono::system_clock::now();
	auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>( now );
	auto value = now_ms.time_since_epoch();
	long duration = value.count();

	// 使用隨機數生成器生成一個隨機數
	std::mt19937 generator( static_cast<unsigned int>(duration) );
	std::uniform_int_distribution<int> distribution( 0, 999999 );
	int random_number = distribution( generator );

	// 組合時間戳和隨機數
	std::stringstream ss;
	ss << "tmp_" << std::setfill( '0' ) << std::setw( 13 ) << duration
		<< "_" << std::setw( 6 ) << random_number;

	return ss.str();
}

inline bool canRW( const fs::path &dir ) {
	try {
		fs::path temp_file = dir / maketmpId();

		// 嘗試寫入檔案
		{
			std::ofstream file( temp_file );
			if ( !file ) return false;
			file << "Test data";
		}

		// 嘗試讀取檔案
		{
			std::ifstream file( temp_file );
			if ( !file ) {
				fs::remove( temp_file );
				return false;
			}
			std::string content;
			std::getline( file, content );
			if ( content != "Test data" ) {
				fs::remove( temp_file );
				return false;
			}
		}

		// 刪除臨時檔案
		fs::remove( temp_file );

		return true;
	}
	catch ( const std::exception & ) {
		return false;
	}
}

inline void ensure(const fs::path &path) {
	if (fs::exists(path)) return;

	std::string path_str = path.string();
	bool ends_with_separator = !path_str.empty() &&
							  (path_str.back() == '/' || path_str.back() == '\\');

	bool is_likely_file = !ends_with_separator &&
						  path.has_filename() &&
						  !path.filename().empty() &&
						  path.filename().string().find('.') != std::string::npos &&
						  path.filename().string().back() != '.';

	if (is_likely_file) {
		fs::path dir = path.parent_path();
		if (!dir.empty() && !fs::exists(dir)) fs::create_directories(dir);
	} else {
		fs::create_directories(path);
	}
}

inline fs::path tmp( bool recheck = false ) {
	static fs::path validated_tmp_path;
	static bool is_initialized = false;

	if ( recheck || !is_initialized ) {
		std::vector<fs::path> potential_paths = {
			fs::path( std::getenv( "TMPDIR" ) ? std::getenv( "TMPDIR" ) : "" ),
			fs::path( std::getenv( "TMP" ) ? std::getenv( "TMP" ) : "" ),
			fs::path( std::getenv( "TEMP" ) ? std::getenv( "TEMP" ) : "" ),
			"/tmp",
		};

		for ( const auto &path: potential_paths ) {
			if ( !path.empty() && fs::exists( path ) && canRW( path ) ) {
				validated_tmp_path = path;
				is_initialized = true;
				return validated_tmp_path;
			}
		}

		throw std::runtime_error( "所有tmp目錄都沒有讀寫權限" );
	}

	return validated_tmp_path;
}

using IFnFind = function<bool( const fs::directory_entry & )>;

inline std::vector<fs::path> find( const fs::path &dir, IFnFind finder ) {
	std::vector<fs::path> result;

	if ( fs::exists( dir ) && fs::is_directory( dir ) ) {
		for ( const auto &entry: fs::directory_iterator( dir ) ) if ( finder( entry ) ) result.push_back( entry.path() );
	}

	return result;
}

inline std::vector<fs::path> dirs( const fs::path &dir ) {

	return find( dir, []( const fs::directory_entry &p ) { return fs::is_directory( p.status() ); } );
}
inline std::vector<fs::path> files( const fs::path &dir ) {

	return find( dir, []( const fs::directory_entry &p ) { return fs::is_regular_file( p.status() ); } );
}

}
