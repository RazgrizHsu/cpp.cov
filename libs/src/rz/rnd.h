#pragma once

#include <random>


namespace rz::rnd {


// 生成4KB的隨機字串
static std::string generateRandomString(size_t length) {
	std::string characters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	std::random_device rd;
	std::mt19937 generator(rd());
	std::uniform_int_distribution<> distribution(0, characters.size() - 1);

	std::string randomString;
	for (size_t i = 0; i < length; ++i) {
		randomString += characters[distribution(generator)];
	}
	return randomString;
}


}
