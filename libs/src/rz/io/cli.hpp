#pragma once

#include <spdlog/fmt/bundled/core.h>
#include <spdlog/fmt/bundled/color.h>
#include <iostream>

namespace rz::io::cli {

//自製的progress, 但還是用indicators比較好用
inline void updateProgress(float progress, std::ostream& out = std::cout) {
	int barWidth = 70;
	int pos = barWidth * progress;

	fmt::memory_buffer buf;
	fmt::format_to(std::back_inserter(buf), "[");
	for (int i = 0; i < barWidth; ++i) {
		if (i < pos) fmt::format_to(std::back_inserter(buf), "=");
		else if (i == pos) fmt::format_to(std::back_inserter(buf), ">");
		else fmt::format_to(std::back_inserter(buf), " ");
	}

#ifdef IS_TESTS
	//lg::info( fmt::to_string(buf) );
	fmt::format_to(std::back_inserter(buf), "] {}%\n", int(progress * 100.0));
#else
	fmt::format_to(std::back_inserter(buf), "] {}%\r", int(progress * 100.0));
#endif

	//out << fmt::to_string(buf);
	out << fmt::to_string(buf) << std::flush;
	out.flush();
}


}
