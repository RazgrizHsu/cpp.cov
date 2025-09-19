#pragma once

#include <codecvt>
#include <spdlog/fmt/fmt.h>

using namespace std;

//------------------------------------------------------------------------
// private
//------------------------------------------------------------------------
namespace rz::str::pv {


inline bool is_in_range( char32_t c, char32_t low, char32_t high ) { return c >= low && c <= high; }

// 日文字符範圍
const std::array<std::pair<char32_t, char32_t>, 3> ranges_jp = {
	std::make_pair( 0x3040, 0x309F ), // Hiragana
	std::make_pair( 0x30A0, 0x30FF ), // Katakana
	std::make_pair( 0x4E00, 0x9FFF )  // Kanji
};

// 簡體中文字符範圍
const std::array<std::pair<char32_t, char32_t>, 3> ranges_cn = {
	std::make_pair( 0x4E00, 0x9FFF ), // CJK Unified Ideographs
	std::make_pair( 0x2E80, 0x2EFF ), // CJK Radicals Supplement
	std::make_pair( 0x2F00, 0x2FDF )  // Kangxi Radicals
};

// 繁體中文字符範圍
const std::array<std::pair<char32_t, char32_t>, 4> ranges_tw = {
	std::make_pair( 0x4E00, 0x9FFF ), // CJK Unified Ideographs
	std::make_pair( 0x2E80, 0x2EFF ), // CJK Radicals Supplement
	std::make_pair( 0x2F00, 0x2FDF ), // Kangxi Radicals
	std::make_pair( 0x3100, 0x312F )  // Bopomofo
};

inline bool is_in_language( char32_t c, const auto &ranges ) {
	return std::any_of( ranges.begin(), ranges.end(),
		[c]( const auto &range ) { return is_in_range( c, range.first, range.second ); } );
}

inline bool contains_char_type( const std::string &str, const auto &ranges ) {
	auto it = str.begin();
	while ( it != str.end() ) {
		char32_t codepoint = 0;
		int bytes = 0;

		if ( ( *it & 0x80 ) == 0 ) { // ASCII character
			++it;
			continue;
		} else if ( ( *it & 0xE0 ) == 0xC0 ) { // 2-byte sequence
			if ( std::distance( it, str.end() ) < 2 ) break;
			codepoint = ( ( *it & 0x1F ) << 6 ) | ( *( it + 1 ) & 0x3F );
			bytes = 2;
		} else if ( ( *it & 0xF0 ) == 0xE0 ) { // 3-byte sequence
			if ( std::distance( it, str.end() ) < 3 ) break;
			codepoint = ( ( *it & 0x0F ) << 12 ) | ( ( *( it + 1 ) & 0x3F ) << 6 ) | ( *( it + 2 ) & 0x3F );
			bytes = 3;
		} else if ( ( *it & 0xF8 ) == 0xF0 ) { // 4-byte sequence
			if ( std::distance( it, str.end() ) < 4 ) break;
			codepoint = ( ( *it & 0x07 ) << 18 ) | ( ( *( it + 1 ) & 0x3F ) << 12 ) | ( ( *( it + 2 ) & 0x3F ) << 6 ) | ( *( it + 3 ) & 0x3F );
			bytes = 4;
		} else {
			++it; // Invalid UTF-8 sequence, skip
			continue;
		}

		if ( is_in_language( codepoint, ranges ) ) { return true; }

		std::advance( it, bytes );
	}
	return false;
}


}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
namespace rz::str {


inline bool hasJP( const std::string &str ) { return pv::contains_char_type( str, pv::ranges_jp ); }
inline bool hasCN( const std::string &str ) { return pv::contains_char_type( str, pv::ranges_cn ); }
inline bool hasTW( const std::string &str ) { return pv::contains_char_type( str, pv::ranges_tw ); }


}
