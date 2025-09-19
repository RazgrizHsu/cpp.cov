#pragma once

// #include <type_traits>
// #include <functional>
// #include <any>
//
//
//
// template<typename T>
// T anyOr( const std::any &a, const T &default_value ) {
// 	const T *result = std::any_cast<T>( &a );
// 	return result ? *result : default_value;
// }


// 不能處理多載
 // template<typename F, typename... Args>
 // auto anyRun( F &&func, Args &&... args ) {
 // 	if constexpr ( std::is_void_v<std::invoke_result_t<F, Args...> > ) {
 // 		std::invoke( std::forward<F>( func ), std::forward<Args>( args )... );
 // 		return std::any{}; // 返回空的 std::any
 // 	}
 // 	else {
 // 		return std::invoke( std::forward<F>( func ), std::forward<Args>( args )... );
 // 	}
 // }


//
// template<typename F, typename... Args>
// auto anyRun( F &&func, Args &&... args )
// 	-> std::conditional_t<
// 		std::is_void_v<std::invoke_result_t<F, Args...> >,
// 		std::any,
// 		std::invoke_result_t<F, Args...>
// 	> {
// 	if constexpr ( std::is_void_v<std::invoke_result_t<F, Args...> > ) {
// 		std::invoke( std::forward<F>( func ), std::forward<Args>( args )... );
// 		return std::any{}; // 返回空的 std::any
// 	}
// 	else {
// 		return std::invoke( std::forward<F>( func ), std::forward<Args>( args )... );
// 	}
// }
