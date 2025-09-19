#include <rz/idx.h>

#include "./defs.h"
#include "./mov.h"
#include <cmds/idx.h>

using namespace rz;
using namespace rz::io;
using namespace cmds::cov;

//==================================================================
//==================================================================
std::map<string, fs::path> buildMapArts( const CovCfg &cc ) {
	using namespace cmds::cov;



	fs::path pRootArts = cc.rootArts;
	if ( !fs::exists( pRootArts ) ) throw std::runtime_error( rz::fmt( "作者根目錄找不到: {}", pRootArts.string() ) );

	auto dirs = io::dir::dirs( pRootArts );

	//填充art names


	std::map<string, fs::path> mapArts;

	for ( const auto &p: dirs ) {

		auto namef = p.filename().string();

		if ( !str::contains( namef, ";" ) ) continue;

		std::istringstream ss( namef );
		std::string nam;

		while ( std::getline( ss, nam, ';' ) ) {
			if ( nam.empty() ) continue;

			nam = str::replaceAll( nam, "_", "" );
			mapArts[nam] = p;
		}
	}

	//for ( const auto &[k,p]: mapArts ) lg::info( "k[{}] {}", k, p.filename().string() );

	return mapArts;
}


void moveArts( const fs::path &pRoot ) {

	using namespace cmds::cov;

	auto cc = CovCfg::load();
	auto mapArts = buildMapArts( *cc );


	for ( const auto &[k,p]: mapArts ) lg::info( "k[{}] {}", k, p.filename().string() );

}



//==================================================================
// main
//==================================================================
void cmds::cov::mv::moveArts( const fs::path &path ) {

	moveArts( path );


	return;
	cmds::run( path, [&]() {

		auto rootDir = IDir::make( path );
		if ( !rootDir ) {
			lg::info( "[mov] path wrong: {}", path.string() );
			return;
		}

		lg::info( "========================================================================" );
		lg::info( "💖 move Dir: {}", path.string() );
		lg::info( "------------------------------------------------------------------------" );

		moveArts( path );

		lg::info( "========================================================================" );
		lg::info( "done" );
		lg::info( "========================================================================" );


	} );
}



void mv::extractSingleFiles( const fs::path &path ) {

	using namespace rz::io;

	cmds::run( path, [&]() {

		auto onFile = []( const fs::path &p ) { return p.filename().string() != ".cov"; }; //略過.cov檔

		auto rootDir = IDir::make( path, { .onCountFile = onFile } );
		if ( !rootDir ) {
			lg::info( "[mov] path wrong: {}", path.string() );
			return;
		}

		lg::info( "========================================================================" );
		lg::info( "💖 single file from Dir: {}", path.string() );
		lg::info( "------------------------------------------------------------------------" );

		auto dirs = rootDir->findDirs( []( const IDir &dir ) -> IFindRst {

			//lg::info( "[dir] {} {}", dir.infos.str(), dir.path.string() );
			auto is = dir.infos;
			if ( is.deep == 0 && is.files == 1 ) return { .include = 1 };

			return { .toDeep = 1, .include = 0 };
		} );

		auto idx = 0;
		for ( auto &dir: dirs ) {
			//lg::info( "dir: {}", dir->path.string() );

			auto fil = dir->files[0];
			if ( fil.extension() != ".mp3" ) continue;

			auto pnew = rootDir->path / rz::fmt( "{}.mp3", dir->path.filename() );

			lg::info( "[{}] file move [{}] -> [{}]", idx++, fil.string(), pnew.string() );

			fs::rename( fil, pnew );
			dir->del();
		}



		lg::info( "========================================================================" );
		lg::info( "done" );
		lg::info( "========================================================================" );


	} );

}
