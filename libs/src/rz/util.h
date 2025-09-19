#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include <spdlog/fmt/fmt.h>

// 為 std::filesystem::path 提供一個特殊的 formatter
namespace fmt {

template <>
struct formatter<std::filesystem::path> : formatter<string_view> {
template <typename FormatContext>
auto format(const std::filesystem::path& p, FormatContext& ctx) {
	return formatter<string_view>::format(p.string(), ctx);
}
};

// 為 std::vector<std::filesystem::path> 提供一個特殊的 formatter
template <>
struct formatter<std::vector<std::filesystem::path>> : formatter<string_view> {
template <typename FormatContext>
auto format(const std::vector<std::filesystem::path>& vec, FormatContext& ctx) {
	std::string result = "[";
	for (size_t i = 0; i < vec.size(); ++i) {
		if (i > 0) {
			result += ", ";
		}
		result += vec[i].string();
	}
	result += "]";
	return formatter<string_view>::format(result, ctx);
}
};

} // namespace fmt

// 如果你仍然需要通用的 vector formatter，可以保留這個
namespace fmt {

template<typename T>
struct formatter<std::vector<T>> : formatter<string_view> {
template<typename FormatContext>
auto format(const std::vector<T>& vec, FormatContext& ctx) {
	std::string result = "[";
	for (size_t i = 0; i < vec.size(); ++i) {
		if (i > 0) {
			result += ", ";
		}
		result += fmt::format("{}", vec[i]);
	}
	result += "]";
	return formatter<string_view>::format(result, ctx);
}
};

} // namespace fmt
namespace rz {
namespace util {


}}

