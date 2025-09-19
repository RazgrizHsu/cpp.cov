#include <iostream>
#include <rz/idx.h>
#include <cmds/idx.h>


using namespace std;

int main( int argc, char *argv[] ) {



	cli::Ap ap( "txt" );

	//------------------------------------------------------------------------
	// 合併files
	//------------------------------------------------------------------------
	ap.cmd( "merge", "合併該資料夾內的檔案為文字檔" )
	.arg<string>( "path", "convert path if not set use current", true )
	.handle( []( const cli::Args &args ) {

		auto path = args.get<fs::path>( "path", fs::current_path() );

		auto dir = rz::io::IDir::make( path );

		auto fils = dir->allFiles();

		stringstream ss;
		for ( const auto &p: fils ) {

			ss << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
			ss << "━ 檔案: " << p.filename() << std::endl;
			ss << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;

			std::ifstream inFile( p, std::ios::binary );
			if ( inFile ) {

				char bom[3] = { 0 };
				inFile.read( bom, 3 );

				// 如果不是 BOM，將檔案指針移回開始
				if ( !( bom[0] == (char)0xEF && bom[1] == (char)0xBB && bom[2] == (char)0xBF ) ) inFile.seekg( 0 );

				// 讀取檔案內容（不包括 BOM）
				ss << inFile.rdbuf();
				inFile.close();
			}
			else {
				ss << "Error: Unable to open file " << p << std::endl;
			}
			ss << std::endl << std::endl;
		}


		auto pto = path / "merge.txt";
		rz::io::fil::save( pto, ss.str() );

	} );


	//------------------------------------------------------------------------
	// main
	//------------------------------------------------------------------------
	ap
		.arg<string>( "path", "convert path if not set use current", true )
		.flag( "-c", "clean all .cov files" )
		.handle( []( const cli::Args &args ) {
			auto path = args.get<fs::path>( "path", fs::current_path() );

		} );


	return ap.run( argc, argv );
}
