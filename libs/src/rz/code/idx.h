#pragma once

#include <chrono>
#include <spdlog/idx.h>


#include "./class.h"
#include "./concept.h"
#include "./progress.h"

namespace rz::code {

template<typename F>
long long measureExecutionTime(F&& lambda) {
	auto start = std::chrono::high_resolution_clock::now();

	try {
		lambda();
	}
	catch( std::exception &ex ) {
		lg::error( "[measure] ex: {}", ex.what() );
	}

	auto end = std::chrono::high_resolution_clock::now();

	return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

}