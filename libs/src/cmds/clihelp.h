#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <functional>
#include <variant>
#include <any>
#include <typeindex>
#include <sstream>
#include <type_traits>
#include <filesystem>


namespace cli {
using namespace std;

class Args;
class Ap;

class Cmd;

// using IFnVoid = std::function<void()>;
// using IHandler = std::variant<std::function<void()>, std::function<int()>, std::function<void( const Args & )>, std::function<int( const Args & )>>;
// using IPrinter = function<void( const string & )>;
//
//
//
// template<typename T>
// std::string getTypeName( const std::optional<T> &optT ) { return optT ? getTypeName( *optT ) : ""; }
//
// inline std::string join( char *argv[], string delimiter = " " ) {
// 	std::string result;
// 	for ( int i = 0; argv[i] != nullptr; ++i ) {
// 		if ( i > 0 ) result += delimiter;
// 		result += argv[i];
// 	}
// 	return result;
// }
//
//
// template<typename Func, typename T>
// concept IFnRetStr = requires( Func f, T arg ) { { f( arg ) } -> std::convertible_to<std::string>; };
//
// template<typename T, IFnRetStr<T> Func>
// std::string join( const std::vector<T> &vecs, Func selector, const std::string &delimiter = " " ) {
// 	std::stringstream ss;
// 	for ( size_t i = 0; i < vecs.size(); ++i ) {
// 		if ( i != 0 ) ss << delimiter;
// 		ss << selector( vecs[i] );
// 	}
// 	return ss.str();
// }
//
//
// static vector<string> splitArguments( const string &argStr ) {
// 	vector<string> args;
// 	istringstream iss( argStr );
// 	string arg;
// 	while ( getline( iss, arg, ',' ) ) args.push_back( arg );
// 	return args;
// }


template<typename T> string toStr( const T &value ) { return to_string( value ); }
template<> inline string toStr<string>( const string &value ) { return value; }

inline string toStr( const std::any &value ) {
	if ( value.type() == typeid( int ) ) return toStr( any_cast<int>( value ) );
	else if ( value.type() == typeid( double ) ) return toStr( any_cast<double>( value ) );
	else if ( value.type() == typeid( string ) ) return toStr( any_cast<string>( value ) );

	return "Unsupported type";
}

template<typename... Args>
string fmtstr( const string &formatStr, Args... args ) {
	vector<string> argVec = { toStr( args )... };
	string result;
	size_t argIndex = 0;
	for ( size_t i = 0; i < formatStr.size(); ++i ) {
		if ( formatStr[i] == '{' && i + 1 < formatStr.size() && formatStr[i + 1] == '}' && argIndex < argVec.size() ) {
			result += argVec[argIndex++];
			++i; // skip '}'
		}
		else { result += formatStr[i]; }
	}
	return result;
}


}
