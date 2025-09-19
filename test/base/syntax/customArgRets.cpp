#include "doctest.h"
#include <functional>
#include <any>
#include <type_traits>
#include <iostream>

struct AVFormatContext {};

template<typename Fn>
concept IFnForAFC = requires( Fn func, AVFormatContext *ctx )
{
	{ func( ctx ) };
};

template<typename Fn>
using IRstForAFC = std::invoke_result_t<Fn, AVFormatContext *>;

template<IFnForAFC Fn>
auto onInfo( const std::string &filePath, Fn &&onAct )
	-> std::conditional_t<std::is_void_v<IRstForAFC<Fn>>, void, IRstForAFC<Fn>> {
	AVFormatContext ctx;

	if constexpr ( std::is_void_v<IRstForAFC<Fn>> ) {
		onAct( &ctx );
		std::cout << "[onInfo:str] Lambda executed without explicit return" << std::endl;
	}
	else {
		auto result = onAct( &ctx );
		if constexpr ( std::is_same_v<IRstForAFC<Fn>, int> ) {
			std::cout << "[onInfo:str] Lambda returned a value: " << result << std::endl;
		}
		else {
			std::cout << "[onInfo:str] Lambda returned a non-int value" << std::endl;
		}
		return result;
	}
}

template<typename T>
const T &getRef( T &&data ) {
	if constexpr ( std::is_pointer_v<std::decay_t<T>> ) return *data;
	else return std::forward<T>( data );
}

template<typename T>
concept IVec = requires( T )
{
	typename std::decay_t<T>::value_type;
	requires std::same_as<std::decay_t<T>, std::vector<typename std::decay_t<T>::value_type>>;
};

template<IVec T, IFnForAFC Func>
auto onInfo( T &&data, Func &&onAct )
	-> std::conditional_t<std::is_void_v<IRstForAFC<Func>>, void, IRstForAFC<Func>>
{
	auto vec = getRef( std::forward<T>( data ) );

	AVFormatContext ctx;
	std::cout << "[onInfo:vec] data: " << vec.size() << std::endl;

	if constexpr ( std::is_void_v<IRstForAFC<Func>> ) {
		onAct( &ctx );
		std::cout << "[onInfo:vec] Lambda executed without explicit return" << std::endl;
	}
	else {
		auto result = onAct( &ctx );
		if constexpr ( std::is_same_v<IRstForAFC<Func>, int> ) {
			std::cout << "[onInfo:vec] Lambda returned a value: " << result << std::endl;
		}
		else {
			std::cout << "[onInfo:vec] Lambda returned a non-int value" << std::endl;
		}
		return result;
	}
}

TEST_CASE( "syntax: fn_customArgRets" ) {

	onInfo( "test.mp4", []( AVFormatContext *ctx ) {
		std::cout << "[lambda:str] no-return" << std::endl;
	} );

	int result = onInfo( "test.mp4", []( AVFormatContext *ctx ) -> int {
		std::cout << "[lambda:str] return-> value" << std::endl;
		return 42;
	} );
	std::cout << "[TEST] Returned value: " << result << std::endl;

	std::vector<unsigned char> vec{ 'a', 'b', 'c' };
	int vecResult = onInfo( vec, []( AVFormatContext *ctx ) -> int {
		std::cout << "[lambda:vec] return-> value" << std::endl;
		return 42;
	} );
	std::cout << "[TEST] Returned vec value: " << vecResult << std::endl;

	onInfo( vec, []( AVFormatContext *ctx ) {
		std::cout << "[lambda:vec] no-return" << std::endl;
	} );

	// This should give a clear compile-time error

	// onInfo("test.mp4", [](const std::string xx) -> int {
	//     std::cout << "[run] return-> value" << std::endl;
	//     return 42;
	// });
}
