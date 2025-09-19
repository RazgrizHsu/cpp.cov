#pragma once

#include <string>
#include <fstream>


namespace rz::io {

namespace fs = std::filesystem;

// openmode: https://www.cplusplus.com/reference/ios/ios_base/openmode/
inline std::ofstream ofstream( std::string path, std::ios::openmode mode = std::ios::openmode() )
{
	std::ofstream outfile;
	outfile.open( path, mode );

	if ( !outfile.is_open() )
	{
		//		char *name = strdup( "/tmp/tmp.TmpFile" );
		//		mkstemp( name );
		//lg::info( "[io] tmp: {}", name );
	}

	return outfile;
}


inline int depths( const fs::path& path ) {
	return std::distance(path.begin(), path.end());
}

}