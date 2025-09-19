#pragma once

#include <codecvt>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <unicode/regex.h>
#include <unicode/unistr.h>

using namespace std;

//------------------------------------------------------------------------
// private
//------------------------------------------------------------------------

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
namespace rz::str::rgx {


class CacheRegex {
private:
	std::unordered_map<std::string, std::unique_ptr<icu::RegexPattern>> cache;
	std::mutex mutex;

public:
	icu::RegexPattern *getPattern( const std::string &regex ) {
		std::lock_guard<std::mutex> lock( mutex );
		if ( auto it = cache.find( regex ); it != cache.end() ) return it->second.get();


		UErrorCode status = U_ZERO_ERROR;
		UParseError parseError;
		auto uRegex = icu::UnicodeString::fromUTF8( regex );
		auto pattern = std::unique_ptr<icu::RegexPattern>( icu::RegexPattern::compile( uRegex, parseError, status ) );

		if ( U_FAILURE( status ) ) {
			std::ostringstream msg;
			msg << "正則表達式編譯失敗: " << u_errorName( status );
			throw std::runtime_error( msg.str() );
		}

		auto *result = pattern.get();
		cache[regex] = std::move( pattern );
		return result;
	}
};

static CacheRegex cacheRgx;

inline bool has( const string &src, const string &target ) {
	UErrorCode status = U_ZERO_ERROR;

	auto uSrc = icu::UnicodeString::fromUTF8( src );

	// 使用快取獲取編譯後的正則表達式
	auto *pattern = cacheRgx.getPattern( target );

	// 創建 matcher 並檢查是否匹配
	std::unique_ptr<icu::RegexMatcher> matcher( pattern->matcher( uSrc, status ) );
	bool result = matcher->find( status );

	if ( U_FAILURE( status ) ) throw std::runtime_error( "正則表達式匹配過程中發生錯誤" );

	return result;
}

inline int count( const string &src, const string &regex ) {
	UErrorCode status = U_ZERO_ERROR;

	auto uSrc = icu::UnicodeString::fromUTF8( src );

	auto *pattern = cacheRgx.getPattern( regex );

	// 創建 matcher
	std::unique_ptr<icu::RegexMatcher> matcher( pattern->matcher( uSrc, status ) );

	int count = 0;
	int32_t start = 0;
	while ( matcher->find( start, status ) ) {
		++count;

		// 獲取並列印匹配的字串
		// icu::UnicodeString match = matcher->group(status);
		// std::string matchStr;
		// match.toUTF8String(matchStr);
		//std::cout << "匹配 #" << count << ": '" << matchStr << "'" << std::endl;

		start = matcher->end( status );
		// 如果匹配的是空字串，手動移動起始位置以避免無限循環
		if ( start == matcher->start( status ) ) {
			++start;
			if ( start > uSrc.length() ) break;
		}
	}

	if ( U_FAILURE( status ) ) throw std::runtime_error( "正則表達式匹配過程中發生錯誤" );

	//std::cout << "總匹配數: " << count << std::endl;

	return count;
}



template<typename T>
string replace( T &s, const string &f, const string &to ) {
	using namespace icu;
	UErrorCode ste = U_ZERO_ERROR;
	UParseError ex;

	auto rgx = icu::UnicodeString::fromUTF8( f );
	RegexPattern *pattern = icu::RegexPattern::compile( rgx, ex, ste );

	if ( U_FAILURE( ste ) ) {
		std::ostringstream msg;
		msg << "正則表達式編譯失敗: " << u_errorName( ste );

		msg << "\n錯誤位置: " << ex.offset;

		UnicodeString preContext( ex.preContext, u_strlen( ex.preContext ) );
		UnicodeString postContext( ex.postContext, u_strlen( ex.postContext ) );

		std::string preContextUtf8, postContextUtf8;
		preContext.toUTF8String( preContextUtf8 );
		postContext.toUTF8String( postContextUtf8 );

		msg << "\n錯誤前文本: " << preContextUtf8;
		msg << "\n錯誤後文本: " << postContextUtf8;
		throw std::runtime_error( msg.str() );
	}

	auto src = UnicodeString::fromUTF8( s );
	auto dst = UnicodeString::fromUTF8( to );

	icu::RegexMatcher *matcher = pattern->matcher( src, ste );
	auto rst = matcher->replaceAll( dst, ste );

	string ret;
	rst.toUTF8String( ret );

	return ret;
}


inline std::string removeKeysWithBraces( const std::string &src, const std::vector<std::string> &keys ) {
	UErrorCode status = U_ZERO_ERROR;
	auto uSrc = icu::UnicodeString::fromUTF8( src );

	std::vector<std::pair<icu::UnicodeString, icu::UnicodeString>> brackets = {
		{ icu::UnicodeString::fromUTF8( "【" ), icu::UnicodeString::fromUTF8( "】" ) },
		{ icu::UnicodeString::fromUTF8( "\\(" ), icu::UnicodeString::fromUTF8( "\\)" ) }
	};

	for ( const auto &keyword: keys ) {
		auto key = icu::UnicodeString::fromUTF8( keyword );
		for ( const auto &[open, close]: brackets ) {
			icu::UnicodeString pattern = open + "[^" + close + "]*" + key + "[^" + close + "]*" + close;
			icu::RegexPattern *regex = icu::RegexPattern::compile( pattern, 0, status );
			if ( U_SUCCESS( status ) ) {
				auto *matcher = regex->matcher( uSrc, status );
				if ( U_SUCCESS( status ) ) uSrc = matcher->replaceAll( icu::UnicodeString( "" ), status );
				delete matcher;
				delete regex;
			}
		}
	}

	std::string result;
	uSrc.toUTF8String( result );
	return result;
}

}
