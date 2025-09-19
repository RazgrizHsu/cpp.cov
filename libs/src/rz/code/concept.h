#pragma once

#include <functional>


namespace rz::code {


template<typename T>
concept IVec = requires( T )
{
	typename std::decay_t<T>::value_type;
	requires std::same_as<std::decay_t<T>, std::vector<typename std::decay_t<T>::value_type> >;
};


template<typename T>
const T& getRef(T&& data) {
	if constexpr (std::is_pointer_v<std::decay_t<T>>)
		return *data;
	else
		return std::forward<T>(data);
}

}
