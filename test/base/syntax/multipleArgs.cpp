#include "doctest.h"

#include "../../../libs/src/rz/idx.h"


#include <iostream>
#include <vector>
#include <filesystem>
#include <type_traits>

namespace fs = std::filesystem;
// 定義結構體 A
struct A {
	int age = 0;
};

// 包裝器模板，用於捕獲大括號初始化
template<typename T>
struct Wrapper {
	T value;
};

// 推導指引，用於從大括號初始化推導出 A 類型
Wrapper(int) -> Wrapper<A>;

template<typename... Args>
std::vector<fs::path> find(Args&&... args)
{
	std::vector<fs::path> result;

	auto process = [&result](const auto& arg) {
		using ArgType = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<ArgType, const char*>) {
			std::cout << "Processing string: " << arg << std::endl;
			// 處理字串參數
		} else if constexpr (std::is_same_v<ArgType, A>) {
			std::cout << "Processing A with age: " << arg.age << std::endl;
			// 處理 A 結構體
		} else if constexpr (std::is_same_v<ArgType, Wrapper<A>>) {
			std::cout << "Processing wrapped A with age: " << arg.value.age << std::endl;
			// 處理包裝的 A 結構體
		}
	};

	(process(std::forward<Args>(args)), ...);

	return result;
}

TEST_CASE( "syntax: multipleArgs" )
{
	//find("*.jpg", {.age = 99} ); //can't
	find( "*.jpg", "b", A{.age = 99} ); //但這沒有編譯安全性檢查
}
