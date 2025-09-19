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
#include <filesystem>
#include <unordered_map>


namespace cli {

class err {

	inline static std::vector<std::string> errs;

public:
	static void add( const std::string &error ) { errs.push_back( error ); }
	static const std::vector<std::string> &all() { return errs; }
	static void clear() { errs.clear(); }
	static bool has() { return !errs.empty(); }
	static std::string allStr() {
		std::stringstream ss;
		for ( const auto &error: errs ) ss << error << "\n";
		return ss.str();
	}
};

}


namespace cli {
using namespace std;

class Args;
class Ap;
class Cmd;


using IFnVoid = function<void()>;
using IHandler = variant<function<void()>, function<int()>, function<void( const Args & )>, function<int( const Args & )>>;
using IFnActOut = function<void( const string & )>;


template<typename T>
inline constexpr bool always_false_v = false;

template<typename T>
concept ValidType = std::disjunction_v<
	std::is_same<T, bool>,
	std::is_same<T, double>,
	std::is_same<T, int>,
	std::is_same<T, long>,
	std::is_same<T, std::string>,
	std::is_same<T, std::filesystem::path>
>;



inline std::string getTypeName( const type_index &idx ) {
	static unordered_map<type_index, string> typeMap = {
		{ typeid( std::string ), "string" },
		{ typeid( int ), "int" },
		{ typeid( unsigned int ), "unsigned int" },
		{ typeid( long ), "long" },
		{ typeid( unsigned long ), "unsigned long" },
		{ typeid( long long ), "long long" },
		{ typeid( unsigned long long ), "unsigned long long" },
		{ typeid( float ), "float" },
		{ typeid( double ), "double" },
		{ typeid( long double ), "long double" },
		{ typeid( char ), "char" },
		{ typeid( unsigned char ), "unsigned char" },
		{ typeid( short ), "short" },
		{ typeid( unsigned short ), "unsigned short" },
		{ typeid( bool ), "bool" },
	};

	auto it = typeMap.find( idx );
	if ( it != typeMap.end() ) { return it->second; }

	return idx.name();
}

namespace pv {
inline IFnActOut &getPrinter() {
	static IFnActOut _p = []( const string &msg ) { cout << msg << "\n"; };
	return _p;
}
}

inline void setPrinter( IFnActOut p ) { pv::getPrinter() = std::move( p ); }
inline void print( const string &msg ) { pv::getPrinter()( msg ); }

template<typename... Args>
void printfmt( const string &format, const Args &... args ) {
	stringstream ss;
	string result = format;
	vector<string> argStrings;
	(argStrings.push_back( ( ss.str( "" ), ss << args, ss.str() ) ), ...);
	size_t replace_pos = 0;
	for ( const auto &arg: argStrings ) {
		replace_pos = result.find( "{}", replace_pos );
		if ( replace_pos != string::npos ) result.replace( replace_pos, 2, arg ), replace_pos += arg.length();
	}
	print( result );
}

inline string join( const deque<string> &elements, const string &delimiter = " " ) {
	stringstream ss;
	for ( size_t i = 0; i < elements.size(); ++i ) {
		if ( i != 0 ) ss << delimiter;
		ss << elements[i];
	}
	return ss.str();
}

inline deque<string> convertArgs( char *argv[], int idx = 1 ) {
	deque<string> args;
	for ( int i = idx; argv[i] != nullptr; i++ ) args.emplace_back( argv[i] );
	return args;
}

inline vector<string> splitArguments( const string &argStr ) {
	vector<string> args;
	istringstream iss( argStr );
	string arg;
	while ( getline( iss, arg, ',' ) ) args.push_back( arg );
	return args;
}



struct IArg {
	string name, desc;
	optional<type_index> type;
	optional<IFnVoid> fn;
	bool opt = false, isFlag = false;
};

class Args {
	unordered_map<string, any> args;

public:
	int length() const { return args.size(); }
	const any &operator[]( const string &key ) const {
		auto it = args.find( key );
		if ( it == args.end() ) throw out_of_range( "Key not found: " + key );
		return it->second;
	}
	void add( string k, type_index t, const string v ) {
		try {
			if ( t == type_index( typeid( int ) ) ) args[k] = any( stoi( v ) );
			else if ( t == type_index( typeid( double ) ) ) args[k] = any( stod( v ) );
			else if ( t == type_index( typeid( bool ) ) ) args[k] = any( v == "true" || v == "1" );
			else if ( t == type_index( typeid( long ) ) ) args[k] = any( stol( v ) );
			else if ( t == type_index( typeid( string ) ) ) args[k] = any( v );
			else if ( t == type_index( typeid( filesystem::path ) ) ) args[k] = any( filesystem::path( v ) );
			else throw invalid_argument( "Unsupported type[" + string( t.name() ) + "]" );
		}
		catch ( const exception &ex ) {
			throw invalid_argument( "Invalid argument (" + k + ") value[" + v + "] for type[" + getTypeName( t ) + "] " + ex.what() );
		}
	}
	template<ValidType T>
	T get( string key ) const {
		auto it = args.find( key );
		if ( it == args.end() ) throw invalid_argument( "NotFound the key \"" + key + "\" in arguments" );
		try { return any_cast<T>( it->second ); }
		catch ( const bad_any_cast &e ) { throw invalid_argument( "Type cast failed: " + string( e.what() ) ); }
	}
	template<ValidType T>
	T get( string key, T defaultValue ) const {
		auto it = args.find( key );
		if ( it == args.end() ) return defaultValue;
		try { return any_cast<T>( it->second ); }
		catch ( const exception &e ) { return defaultValue; }
	}
	template<ValidType T>
	optional<T> getOpt( string key ) const {
		auto it = args.find( key );
		if ( it == args.end() ) return nullopt;
		try { return any_cast<T>( it->second ); }
		catch ( const bad_any_cast &e ) { return nullopt; }
	}

};

inline optional<int> execute( const Args &_arg, const optional<IHandler> &func ) {
	optional<int> result;
	if ( func ) {
		visit( [&]( auto &&f ) {
			using T = decay_t<decltype(f)>;
			if constexpr ( is_same_v<T, function<void()>> ) f();
			else if constexpr ( is_same_v<T, function<int()>> ) result = f();
			else if constexpr ( is_same_v<T, function<void( const Args & )>> ) f( _arg );
			else if constexpr ( is_same_v<T, function<int( const Args & )>> ) result = f( _arg );
		}, *func );
	}
	return result;
}

class BaseCmd {
protected:
	vector<IArg> _args;
	optional<IHandler> _hdl;

public:
	virtual ~BaseCmd() = default;
	template<ValidType T>
	BaseCmd &arg( const string &name, const string &desc = "", bool optional = false ) {
		_args.push_back( { name, desc, type_index( typeid( T ) ), nullopt, optional } );
		return *this;
	}
	BaseCmd &flag( const string &name, const string &desc = "" ) {
		_args.push_back( { name, desc, type_index( typeid( bool ) ), nullopt, true, true } );
		return *this;
	}
	BaseCmd &arg( const string &names, const string &desc, IFnVoid fn ) {
		for ( const auto &key: splitArguments( names ) ) _args.push_back( { key, desc, nullopt, fn } );
		return *this;
	}

	template<typename F>
	BaseCmd &handle( F &&f ) {
		if constexpr ( is_invocable_r_v<void, F> ) {
			_hdl = function<void()>( std::forward<F>( f ) );
		}
		else if constexpr ( is_invocable_r_v<int, F> ) {
			_hdl = function<int()>( std::forward<F>( f ) );
		}
		else if constexpr ( is_invocable_r_v<void, F, const Args&> ) {
			_hdl = function<void( const Args & )>( std::forward<F>( f ) );
		}
		else if constexpr ( is_invocable_r_v<int, F, const Args&> ) {
			_hdl = function<int( const Args & )>( std::forward<F>( f ) );
		}
		else {
			static_assert( always_false_v<F>, "Unsupported function signature for handle" );
		}
		return *this;
	}
};

class Cmd : public BaseCmd {
	friend Ap;


public:
	// 禁用複製
	Cmd(const Cmd&) = delete;
	Cmd& operator=(const Cmd&) = delete;

	// 添加移動操作
	Cmd(Cmd&& other) noexcept :
		BaseCmd(std::move(other)),
		_name(std::move(other._name)),
		_desc(std::move(other._desc)),
		_subs(std::move(other._subs)) {}

	Cmd& operator=(Cmd&& other) noexcept {
		if (this != &other) {
			BaseCmd::operator=(std::move(other));
			_name = std::move(other._name);
			_desc = std::move(other._desc);
			_subs = std::move(other._subs);
		}
		return *this;
	}

protected:
	string _name, _desc;
	unordered_map<string, unique_ptr<Cmd>> _subs;

	virtual int printUsage( int ret = 1 ) const {
		string txt = "Usage: " + _name + " ";
		for ( const auto &arg: _args ) txt += arg.name + " ";
		txt += "\nDescription: " + _desc + "\nArgs:\n";
		for ( const auto &arg: _args ) {
			if ( arg.fn ) continue;
			txt += "  " + arg.name + " (" + ( arg.isFlag ? "flag" : getTypeName( *arg.type ) ) + ")";
			if ( arg.opt || arg.isFlag ) txt += " (optional)";
			txt += "\n";
		}
		if ( !_subs.empty() ) {
			txt += "Subs:\n";
			for ( const auto &[name, sub]: _subs ) txt += "  " + name + " - " + sub->_desc + "\n";
		}
		print( txt );
		return ret;
	}

public:
	Cmd( string name, string desc ) : _name( std::move( name ) ), _desc( std::move( desc ) ) {}
	Cmd &cmd( const string &name, const string &desc ) {
		auto it = _subs.emplace( name, std::make_unique<Cmd>( name, desc ) ).first;
		return *it->second;
	}

protected:
	int exec( const deque<string> &rawArgs ) const {
		Args args;
		bool hasRequiredArgs = true;
		bool allArgsOptional = true;

		for ( size_t i = 0, j = 0; i < _args.size(); ++i ) {
			const auto &ag = _args[i];
			if ( ag.fn ) continue;

			if ( ag.isFlag ) {
				args.add( ag.name, type_index( typeid( bool ) ), find( rawArgs.begin(), rawArgs.end(), ag.name ) != rawArgs.end() ? "true" : "false" );
				continue;
			}

			if ( !ag.opt ) allArgsOptional = false;

			if ( j < rawArgs.size() && rawArgs[j][0] != '-' ) { // 確保不會將標誌誤解為參數值
				try {
					if ( !ag.type ) {
						err::add( "Invalid type for argument " + ag.name );
						return printUsage();
					}
					args.add( ag.name, *ag.type, rawArgs[j] );
					++j;
				}
				catch ( const std::exception &e ) {
					err::add( "Error adding argument " + ag.name + ": " + e.what() );
					return printUsage();
				} catch ( ... ) {
					err::add( "Unknown error adding argument " + ag.name );
					return printUsage();
				}
			}
			else if ( !ag.opt ) {
				hasRequiredArgs = false;
				err::add( "Missing required argument " + ag.name );
				return printUsage();
			}
		}

		if ( _hdl && ( allArgsOptional || hasRequiredArgs ) ) {
			try {
				return cli::execute( args, _hdl ).value_or( 0 );
			}
			catch ( const std::exception &e ) {
				err::add( "Error in handler execution: " + string( e.what() ) );
				return 98;
			} catch ( ... ) {
				err::add( "Unknown error in handler execution" );
				return 99;
			}
		}

		if ( _subs.empty() && rawArgs.empty() && !allArgsOptional ) {
			err::add( "No arguments provided and not all args are optional" );
			return printUsage();
		}

		return printUsage();
	}
};

class Ap : public Cmd {
public:
	Ap( string name, string desc = "" ) : Cmd( std::move( name ), std::move( desc ) ) {}
	int run( int argc, char *argv[] ) const {
		auto args = convertArgs( argv );
		for ( const auto &ag: _args ) {
			if ( ag.fn && find( args.begin(), args.end(), ag.name ) != args.end() ) ag.fn.value()();
		}
		if ( !args.empty() ) {
			auto it = _subs.find( args.front() );
			if ( it != _subs.end() ) {
				args.pop_front();
				return it->second->exec( args );
			}
		}
		return this->exec( args );
	}
};
}
