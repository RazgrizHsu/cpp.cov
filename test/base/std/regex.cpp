#include "doctest.h"
#include <chrono>
#include <string>

#include <fstream>
#include <iostream>
#include <vector>

#include <unicode/unistr.h>
#include <unicode/regex.h>


#include "../_utils.hpp"
#include "rz/idx.h"

using namespace rz::str;

TEST_CASE( "regex: move" ) {

	auto src = "[TEST][test123] 【CCD】ZZZ";

	auto dst = rgx::replace( src, R"((.*)(\[[a-zA-Z_0-9]+\])(.*))", "$1$3 $2" );
	lg::info( "\n[src] {}\n[dst] {}\n", src, dst );

	REQUIRE_EQ( dst, "[TEST] 【CCD】ZZZ [test123]" );
}

TEST_CASE( "regex: rename" ) {

	auto src = "!!!(HEAD)_name";
	auto dst = "!!!_name(HEAD)";
	auto regstr = R"(^([!?]*)(\([^\)]+\))([ ]*)([^\n]+))";

	auto rst = rgx::replace( src, regstr, "$1$4$2" );

	lg::info( "rst: {}", rst );
	REQUIRE_EQ( rst, dst );
}

std::string removeVoiceTags( const string &src ) {

	auto str = src;
	vector<string> rgx_replaces = {
		R"(【[^】]*測試[^】]*】)",
	};

	for ( auto &rgs: rgx_replaces ) str = rz::str::rgx::replace( str, rgs, "" );

	return str;
}

TEST_CASE( "regex: rename_voice" ) {

	vector<string> strs = {
		"[ABC123] 【ZZZ】test [BBC999]",
	};

	vector<string> rgx_replaces = {
		R"(【[^】]*ZZZ[^】]*】)",
	};

	for ( auto &s: strs ) {
		auto str = s;
		auto prev = str;

		str = removeVoiceTags( str );

		//for ( auto &rgs: rgx_replaces ) str = rz::str::rgx::replace( str, rgs, "" );

		if ( prev != str ) {
			lg::info( "------------------------------------" );
			lg::info( "原始字: {}", prev );
			lg::info( "取代後: {}", str );
		}
	}
}

TEST_CASE( "regex: base" ) {

	//std::locale old;
	//std::locale::global( std::locale( "en_US.UTF-8" ) );
	//setlocale(LC_ALL, "en_US.UTF-8");

	std::vector<icu::UnicodeString> strs = {
		"[TEST] 【測試】this test [ZZ00123]"
	};

	std::vector<icu::UnicodeString> regs_repl = {
		R"(【[^】]*測試[^】]*】)",
	};

	for ( auto &s: strs ) {
		auto str = s;

		for ( auto &r: regs_repl ) {
			UErrorCode status = U_ZERO_ERROR;
			icu::RegexPattern *pattern = icu::RegexPattern::compile( r, 0, status );
			if ( U_SUCCESS( status ) ) {
				icu::UnicodeString prev = str;
				icu::RegexMatcher *matcher = pattern->matcher( str, status );

				str = matcher->replaceAll( "", status );


				if ( prev != str ) {
					std::string rUtf8, sUtf8, strUtf8;
					r.toUTF8String( rUtf8 );
					s.toUTF8String( sUtf8 );
					str.toUTF8String( strUtf8 );

					lg::info( "=======================! 修改! reg: {}", rUtf8 );
					lg::info( "原始字: {}", sUtf8 );
					lg::info( "取代後: {}", strUtf8 );
				}
			}
		}

	}
}



std::string RenameBy( const std::string &path, bool isIgnoreHeadSymbol = false ) {
	std::string name = path;
	std::regex pattern;
	std::string replacement;

	if ( isIgnoreHeadSymbol ) {
		pattern = std::regex( R"(^([!?]*)(\([^\)]+\))([ ]*)([^\n]+))" );
		replacement = "$1$4$2";
	}
	else {
		pattern = std::regex( R"(^([!?]*)(\([^\)]+\))([ |_]*)([^\n]+))" );
		replacement = "$1$4$2";
	}

	name = std::regex_replace( name, pattern, replacement );

	if ( name.length() >= 4 && name.substr( name.length() - 4 ) == ".zip" ) {
		name = name.substr( 0, name.length() - 4 );
	}

	return name;
}

TEST_CASE( "regex: rename_v1" ) {
	auto n1 = "!!![712][ids] test1 [ZZZ00123]";
	lg::info( "n1: {}", n1 );
	lg::info( "n1: {}", RenameBy( n1 ) );
	lg::info( "n1: {}", RenameBy( n1, true ) );
}
