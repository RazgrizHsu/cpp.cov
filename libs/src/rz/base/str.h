#pragma once

#include <cstdarg>
#include <string>
#include <vector>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cctype>

#include <spdlog/fmt/fmt.h>

#include <unicode/unistr.h>
#include <unicode/utf8.h>
#include <unicode/regex.h>
#include <unicode/uchar.h>

using namespace std;

//------------------------------------------------------------------------
// private
//------------------------------------------------------------------------
namespace rz::pv {


}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
namespace rz {

// template<typename... Args>
// using format_t = decltype(::fmt::format( std::declval<Args>()... ));
//
// template<typename... Args>
// auto fmt( Args &&... args ) -> format_t<Args...> { return ::fmt::format( std::forward<Args>( args )... ); }

template<typename... Args>
concept Formattable = requires( std::string_view fmt_str, Args... args )
{
	fmt::format( fmt_str, std::declval<Args>()... );
};

template<typename Fmt, typename... Args>
	requires Formattable<Args...> && std::convertible_to<Fmt, std::string_view>
auto fmt( Fmt &&format_str, Args &&... args ) {
	return fmt::format( std::forward<Fmt>( format_str ), std::forward<Args>( args )... );
}



static std::string vfmt( const char *fmt, va_list args ) {
	int size = 1024;
	std::vector<char> buf( size );

	while ( true ) {
		va_list args_copy;
		va_copy( args_copy, args );
		int n = vsnprintf( buf.data(), size, fmt, args_copy );
		va_end( args_copy );

		if ( n > -1 && n < size ) return std::string( buf.data(), n );
		if ( n > -1 ) size = n + 1; // 如果緩衝區太小，增加緩衝區大小
		else size *= 2;

		buf.resize( size );
	}
}


// static std::string escapeShell(const std::string &arg) {
// 	// Convert UTF-8 to ICU's UnicodeString
// 	icu::UnicodeString unistr = icu::UnicodeString::fromUTF8(arg);
//
// 	icu::UnicodeString escaped = "\"";
// 	for (int32_t i = 0; i < unistr.length(); ++i) {
// 		UChar32 c = unistr.char32At(i);
// 		if (c == '"' || c == '$' || c == '`' || c == '\\') {
// 			escaped += '\\';
// 		}
// 		escaped += c;
// 	}
// 	escaped += "\"";
//
// 	// Convert back to UTF-8
// 	std::string result;
// 	escaped.toUTF8String(result);
// 	return result;
// }

#include <string>

static std::string escapeShell( const std::string &arg ) {
	UErrorCode status = U_ZERO_ERROR;
	icu::UnicodeString uarg = icu::UnicodeString::fromUTF8( arg );

	// 創建正則表達式物件
	icu::RegexPattern *pattern = icu::RegexPattern::compile(
		icu::UnicodeString::fromUTF8( "[\"$`\\\\]" ), 0, status );

	if ( U_FAILURE( status ) ) {
		// 處理錯誤
		delete pattern;
		return arg; // 返回原始字串
	}

	// 創建正則替換物件
	icu::RegexMatcher *matcher = pattern->matcher( uarg, status );

	if ( U_FAILURE( status ) ) {
		// 處理錯誤
		delete pattern;
		delete matcher;
		return arg; // 返回原始字串
	}

	// 執行替換
	icu::UnicodeString result = matcher->replaceAll( "\\\\$0", status );

	// 清理
	delete pattern;
	delete matcher;

	if ( U_FAILURE( status ) ) {
		// 處理錯誤
		return arg; // 返回原始字串
	}

	// 轉換回 UTF-8 並添加引號
	std::string escaped;
	result.toUTF8String( escaped );
	return "\"" + escaped + "\"";
}

static std::string escapeShell( const std::filesystem::path &arg ) {
	std::string str = arg.string();
	return escapeShell( str );
}

}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
namespace rz::str {

inline bool contains( const string &s, const string &t ) { return s.find( t ) != string::npos; }
inline bool contains( const char* s, const string &t, bool ignoreCase = false ) {
	string str = s;
	string target = t;
	if ( ignoreCase ) {
		ranges::transform( str, str.begin(), ::tolower );
		ranges::transform( target, target.begin(), ::tolower );
	}
	return str.find( target ) != string::npos;
}



template<typename T>
bool contains( const string &s, const vector<T> &ts, bool ignoreCase = false ) {
	for ( const auto &it: ts ) {
		string str = s;
		string target = it;
		if ( ignoreCase ) {
			ranges::transform( str, str.begin(), ::tolower );
			ranges::transform( target, target.begin(), ::tolower );
		}
		if ( str.find( target ) != string::npos ) return true;
	}
	return false;
}

template<typename T>
bool has(const vector<T> &vec, const T &element, bool ignoreCase = false) {
	if (!ignoreCase) {
		return std::find(vec.begin(), vec.end(), element) != vec.end();
	} else {
		if constexpr (std::is_convertible_v<T, std::string>) {
			std::string elem_lower = element;
			ranges::transform(elem_lower, elem_lower.begin(), ::tolower);

			for (const auto &item : vec) {
				std::string item_lower = item;
				ranges::transform(item_lower, item_lower.begin(), ::tolower);
				if (item_lower == elem_lower) {
					return true;
				}
			}
			return false;
		} else {
			// 非字串類型，直接比較
			return std::find(vec.begin(), vec.end(), element) != vec.end();
		}
	}
}

template<typename Container>
bool contains(const Container& container, const std::string& target, bool ignoreCase = false) {
	for (const auto& str : container) {
		std::string s = str;
		std::string t = target;
		if (ignoreCase) {
			std::ranges::transform(s, s.begin(), ::tolower);
			std::ranges::transform(t, t.begin(), ::tolower);
		}
		if (s.find(t) != std::string::npos) return true;
	}
	return false;
}

inline string replaceAll( string &s, const string &f, const string &to ) {
	size_t pos = 0;
	while ( ( pos = s.find( f, pos ) ) != string::npos ) {
		s.replace( pos, f.length(), to );
		pos += to.length();
	}
	return s;
}

//右值
inline std::string replaceAll( std::string &&s, const std::string &from, const std::string &to ) {
	return replaceAll( s, from, to ); // 複用左值引用版本
}

//to_string的短版
template<class T> auto str( T &&t ) { return std::to_string( std::forward<T>( t ) ); }


template<class T>
inline string pad( T &&str, int count = 5 ) { return fmt( "{:>{}}", str, count ); }



inline std::vector<std::string> splitNewLines( const std::string &line ) {
	std::vector<std::string> result;
	std::istringstream iss( line );
	std::string token;

	while ( std::getline( iss, token, '\n' ) ) {
		if ( !token.empty() ) result.push_back( token );
	}

	return result;
}

inline std::string join( const std::vector<std::string> &vec, const std::string &delim = "," ) {
	std::ostringstream result;
	if ( !vec.empty() ) {
		result << vec[0];
		std::for_each( vec.begin() + 1, vec.end(), [&]( const std::string &s ) {
			result << delim << s;
		} );
	}
	return result.str();
}
inline std::string join( const std::vector<std::filesystem::path> &vec, const std::string &delim = "," ) {
	std::ostringstream result;
	if ( !vec.empty() ) {
		result << vec[0].string();
		std::for_each( vec.begin() + 1, vec.end(), [&]( const std::filesystem::path &s ) {
			result << delim << s.string();
		} );
	}
	return result.str();
}
inline std::string join( const std::vector<std::filesystem::path> &vec,
	const std::string &delim,
	const std::function<std::string( const std::filesystem::path & )> &onItem ) {
	std::ostringstream result;
	if ( !vec.empty() ) {
		result << onItem( vec[0] );
		std::for_each( vec.begin() + 1, vec.end(), [&]( const std::filesystem::path &s ) {
			result << delim << onItem( s );
		} );
	}
	return result.str();
}


inline int count( const std::string &str ) {
	icu::UnicodeString ustr = icu::UnicodeString::fromUTF8( str );
	int32_t length = ustr.length();
	int32_t width = 0;

	for ( int32_t i = 0; i < length; ++i ) {
		UChar32 c = ustr.char32At( i );
		int32_t ea_width = u_getIntPropertyValue( c, UCHAR_EAST_ASIAN_WIDTH );
		width += ( ea_width == U_EA_FULLWIDTH || ea_width == U_EA_WIDE ) ? 2 : 1;
		if ( c > 0xFFFF ) ++i; // 跳過代理對的第二個碼元
	}

	return width;
}

inline std::string limit( const std::string &str, int size = 10 ) {
	if ( count( str ) <= size ) return str;

	icu::UnicodeString ustr = icu::UnicodeString::fromUTF8( str );
	int32_t length = ustr.length();
	int32_t width = 0;
	int32_t i;

	for ( i = 0; i < length; ++i ) {
		UChar32 c = ustr.char32At( i );
		int32_t ea_width = u_getIntPropertyValue( c, UCHAR_EAST_ASIAN_WIDTH );
		int char_width = ( ea_width == U_EA_FULLWIDTH || ea_width == U_EA_WIDE ) ? 2 : 1;
		if ( width + char_width > size - 3 ) break;
		width += char_width;
		if ( c > 0xFFFF ) ++i; // 跳過代理對的第二個碼元
	}

	icu::UnicodeString result = ustr.tempSubString( 0, i );
	result += "..";

	std::string utf8_result;
	result.toUTF8String( utf8_result );
	return utf8_result;
}


inline std::string limit( const std::string &str, int left, int right ) {

	if ( right == 0 ) right = left;

	icu::UnicodeString ustr = icu::UnicodeString::fromUTF8( str );
	int32_t length = ustr.length();
	int32_t leftWidth = 0, rightWidth = 0;
	int32_t leftIndex = 0, rightIndex = length;

	// 處理左邊
	for ( int32_t i = 0; i < length; ++i ) {
		UChar32 c = ustr.char32At( i );
		int32_t ea_width = u_getIntPropertyValue( c, UCHAR_EAST_ASIAN_WIDTH );
		int char_width = ( ea_width == U_EA_FULLWIDTH || ea_width == U_EA_WIDE ) ? 2 : 1;
		if ( leftWidth + char_width > left ) break;
		leftWidth += char_width;
		leftIndex = i + 1;
		if ( c > 0xFFFF ) ++i; // 跳過代理對的第二個碼元
	}

	// 處理右邊
	for ( int32_t i = length - 1; i >= leftIndex; --i ) {
		UChar32 c = ustr.char32At( i );
		int32_t ea_width = u_getIntPropertyValue( c, UCHAR_EAST_ASIAN_WIDTH );
		int char_width = ( ea_width == U_EA_FULLWIDTH || ea_width == U_EA_WIDE ) ? 2 : 1;
		if ( rightWidth + char_width > right ) break;
		rightWidth += char_width;
		rightIndex = i;
		if ( c > 0xFFFF ) --i; // 跳過代理對的第一個碼元
	}

	// 如果字串總長度不超過 left + right，直接返回原字串
	if ( leftIndex >= rightIndex ) {
		return str;
	}

	// 構建結果
	icu::UnicodeString result = ustr.tempSubString( 0, leftIndex );
	result += "...";
	result += ustr.tempSubString( rightIndex );

	std::string utf8_result;
	result.toUTF8String( utf8_result );
	return utf8_result;
}

inline std::filesystem::path limitPath( const std::filesystem::path &path, int left = 10, int right = 10 ) {
	std::vector<std::filesystem::path> components;
	for ( const auto &component: path ) {
		components.push_back( component );
	}

	std::filesystem::path result;
	for ( size_t i = 0; i < components.size(); ++i ) {
		if ( i < components.size() - 1 ) {
			// 對除了最後一個組件之外的所有組件應用限制
			result /= limit( components[i].string(), left, right );
		}
		else {
			// 最後一個組件不做修改
			result /= components[i];
		}
	}

	// 保留原始路徑的根名稱（如果有的話）
	if ( path.has_root_name() ) {
		result = path.root_name() / result;
	}

	// 保留原始路徑的根目錄（如果有的話）
	if ( path.has_root_directory() ) {
		result = result.root_path() / result.relative_path();
	}

	return result;
}

}
