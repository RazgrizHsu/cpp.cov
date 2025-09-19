#include <chrono>
#include <string>

#include "../_utils.hpp"
#include "rz/idx.h"


#include "doctest.h"
#include <ctime>
#include <filesystem>


TEST_CASE( "io: base" ) {
	std::string currentTime = rz::dt::nowTimeStr();
}


TEST_CASE( "io: dirs" )
{
	auto path = std::filesystem::path("/Volumes/tmp/vo");

	auto dir = rz::io::IDir::make( path );
	if( !dir ) {
		lg::warn( "not exist: {}", path );
		return;
	}

	lg::info("dir: {}", dir->str() );

	//auto rst = dir->finds(".mp3", { .fuzzy = 1 });
	//lg::info("found files count[{}] {}", rst.size(), rst);

	for( auto &d : dir->dirs ) {

		if( !d->infos.files ) {
			lg::info( "空的資料夾dir: {}", d->path );

			// if( auto ret = d->del() ) lg::info( "刪除異常! {}", *ret );
			// else lg::info( "刪除資料夾: {}", d->path );
		}
	}
}


TEST_CASE( "io: dir_find" )
{
	auto path = std::filesystem::path("/Volumes/tmp/vo");

	lg::info( "start search.." );
	auto dir = rz::io::IDir::make( path );
	if( !dir ) { lg::warn( "not exist: {}", path ); return; }

	lg::info( "dir {}", dir->str() );
	auto dirs = dir->findD( "RJ" );

	lg::info( "found[ {} ]", dirs.size() );
}



TEST_CASE( "io: dir_findAny" )
{
	auto path = std::filesystem::path("/Volumes/tmp/vo");

	lg::info( "start search.." );
	auto dir = rz::io::IDir::make( path );
	if( !dir ) { lg::warn( "not exist: {}", path ); return; }

	lg::info( "dir {}", dir->str() );
	auto dirs = dir->find( "*.jpg", "*.png", "*.txt" );

	lg::info( "found[ {} ]", dirs.size() );
}
