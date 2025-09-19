#include <chrono>
#include <string>

#include "../_utils.hpp"
#include "rz/idx.h"


#include "doctest.h"
#include <ctime>
#include <filesystem>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <stdexcept>

std::string mergeDirectoryFiles(const std::string &dir)
{
	std::ostringstream result;

	try
	{
		for (const auto &entry: std::filesystem::directory_iterator(dir))
		{
			if (entry.is_regular_file())
			{
				std::ifstream file(entry.path(), std::ios::in);
				if (file)
				{
					result << "/** " << entry.path().filename().string() << " ========================================================================**/\n";
					result << file.rdbuf() << "\n";
					file.close();
				}
				else
				{
					std::cerr << "Warning: Unable to open file: " << entry.path() << std::endl;
				}
			}
		}
	}
	catch (const std::filesystem::filesystem_error &e)
	{
		throw std::runtime_error("Error accessing directory: " + std::string(e.what()));
	}

	return result.str();
}


TEST_CASE( "io: mergeDirFiles" )
{
	auto path = "/Volumes/tmp/mp3gain-master/src";

	auto rst = mergeDirectoryFiles(path);

	rz::io::fil::save( "/Volumes/tmp/mp3gain-master/merge.txt", rst );
}
