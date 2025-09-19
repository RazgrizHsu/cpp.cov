#include "doctest.h"
#include <string>
#include <iostream>
#include <filesystem>


#include "../utils.hpp"
#include "rz/idx.h"

#include <archive.h>
#include <archive_entry.h>






TEST_CASE( "archive: withtypes" )
{
	using namespace rz::io::arc;

	lg::info( "TEST: start" );

	auto path = "/Volumes/tmp/test/test/test.zip";

	//auto vec = rz::io::fil::read( path );
	//extract( path );

	// extract( path, []( IArchive &arc, IArcEntry *entry ) {
	//
	// 	std::cout << std::string( 40, '-' ) << std::endl;
	// 	std::cout << entry->path.string() << std::endl;
	//
	// });

	lg::info( "TEST: doen" );
}