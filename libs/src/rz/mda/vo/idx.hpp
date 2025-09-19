#pragma once


#include <rz/mda/_common/conv.h>
#include <rz/sys/cmdr.h>

#include "./conv.h"
#include "./gain.hpp"


namespace rz::mda::vo {

//using sptr = std::shared_ptr;

namespace fs = std::filesystem;


inline bool isMatchDirName( const fs::path &path ) {

	auto name = path.filename().string();
	auto count = rz::str::rgx::count( name, R"(\[[^\]]+\])" );

	return count >= 2;
}

inline std::string removeNoNeeds( const vector<string> &rmKeys, const string &src ) {

	auto str = src;
	vector<string> keys = rmKeys;

	str = rz::str::rgx::removeKeysWithBraces( str, keys );

	str = rz::str::rgx::replace( str, "【[^】]{1,12}】", "" );

	return str;
}

struct opts {

	int kbps = 192;
	string txt = "conv:";

};

inline void convM4A( const fs::path &psrc, const fs::path &pout, opts opts = { .kbps = 192 } ) {

	auto pin = psrc.string();
	auto pou = pout;

	if ( fs::exists( pou ) ) fs::remove( pou );

	auto cmd = rz::fmt( R"(ffmpeg -i {} -vn -b:a {}k -c:a aac -f ipod {})", escapeShell( pin ), opts.kbps, escapeShell( pou ) );

	//lg::info( "cmd: [{}]", cmd );

	auto ret = mda::cmdConv( cmd, opts.txt );
}

inline void convM4A( const fs::path &psrc, opts opts = {} ) {
	auto pout = psrc.parent_path() / ( psrc.stem().string() + ".m4a" );
	convM4A( psrc, pout, opts );
}

inline void convMP3( const fs::path &psrc, const fs::path &pout, opts opts = { .kbps = 128 } ) {

	auto pin = psrc.string();
	auto pou = pout;

	if ( fs::exists( pou ) ) fs::remove( pou );

	auto cmd = rz::fmt( R"(ffmpeg -i {} -vn -ar 44100 -ac 2 -b:a {}k -f mp3 {})", escapeShell( pin ), opts.kbps, escapeShell( pou ) );

	//lg::info( "cmd:\n{}", cmd );

	auto ret = mda::cmdConv( cmd, opts.txt );
	if ( ret ) {
		auto logs = ret.value();
		auto last = logs.back();

		for ( auto &s: logs ) lg::warn( "--> {}", s );

		if ( last.find( "ret:" ) != std::string::npos ) throw std::runtime_error( rz::fmt( "轉檔失敗, 回傳code[{}]", last.substr( 4 ) ) );

		throw std::runtime_error( rz::fmt( "轉檔失敗, 回傳: {}", last ) );
	}
}

inline void convMP3( const fs::path &psrc, opts opts = {} ) {
	auto pout = psrc.parent_path() / ( psrc.stem().string() + ".mp3" );
	convMP3( psrc, pout, opts );
}

//mkdir -p swapped_channels && for file in *.mp3; do [ -f "$file" ] && ffmpeg -i "$file" -filter:a "pan=stereo|c0=c1|c1=c0" "swapped_channels/${file%.*}_swapped.mp3" -y && echo "處理成功: $file" || echo "處理失敗: $file"; done && echo "所有檔案處理完成。結果保存在 'swapped_channels' 目錄中。"
//


inline void swapLR( const fs::path &psrc, const fs::path &pout, opts opts = {} ) {

	auto pin = psrc.string();
	auto pou = pout;

	if ( fs::exists( pou ) ) fs::remove( pou );

	auto cmd = rz::fmt( R"(ffmpeg -i {} -filter:a "pan=stereo|c0=c1|c1=c0" -f mp3 {})", escapeShell( pin ), opts.kbps, escapeShell( pou ) );

	//lg::info( "cmd:\n{}", cmd );

	auto ret = mda::cmdConv( cmd, opts.txt );
	if ( ret ) {
		auto logs = ret.value();
		auto last = logs.back();

		for ( auto &s: logs ) lg::warn( "--> {}", s );

		if ( last.find( "ret:" ) != std::string::npos ) throw std::runtime_error( rz::fmt( "轉檔失敗, 回傳code[{}]", last.substr( 4 ) ) );

		throw std::runtime_error( rz::fmt( "轉檔失敗, 回傳: {}", last ) );
	}
}

inline void swapLR( const fs::path &psrc, opts opts = {} ) {
	auto pout = psrc.parent_path() / ( psrc.stem().string() + ".mp3" );
	swapLR( psrc, pout, opts );
}



}
