#include <fstream>
#include <filesystem>
#include <sstream>

#include "./idir.h"

namespace rz::io {


std::string IDir::str() const {
	std::stringstream ss;
	ss << "IDir{";
	ss << "path:" << path.string() << ", files:" << files.size() << ", ";
	ss << "Infos: " << infos;
	ss << "}";
	return ss.str();
}



}
