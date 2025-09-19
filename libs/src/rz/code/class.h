#pragma once

#include <functional>


namespace rz::code {

template<typename T>
class PropGet {
public:
	PropGet(std::function<T()> getter) : get_(getter) {}

	T operator()() const { return get_(); }

	operator std::string() const { return std::to_string(get_()); }
	std::string toStr() const { return std::to_string(get_()); }

private:
	std::function<T()> get_;
};

template<typename T>
class Prop : public PropGet<T> {
public:
	Prop(std::function<T()> getter, std::function<void(T)> setter)
		: PropGet<T>(getter), set_(setter) {}

	Prop<T>& operator=(T value) {
		set_(value);
		return *this;
	}

private:
	std::function<void(T)> set_;
};


}