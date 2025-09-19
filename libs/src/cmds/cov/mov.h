#pragma once

namespace cmds::cov::mv {

void moveArts( const fs::path &proot );

void extractSingleFiles( const fs::path &path );

//------------------------------------------------------------------------
// voices
//------------------------------------------------------------------------

inline void voiceRenamePicIfNameClose( const fs::path &proot ) {
	using namespace rz::io;
	auto droot = IDir::make( proot );

	auto *fils = &droot->files;

	auto serVo = false, serImg = false;

	vector<fs::path> pimgs;

	for ( auto &pf: *fils ) {
		auto nmf = pf.filename().string();
		auto ext = pf.extension().string();

		if ( rz::str::rgx::has( nmf, "^[0-9]{1,}" ) ) {

			if ( ext == ".mp3" ) serVo = true;
			if ( ext == ".webp" ) {
				serImg = true;
				pimgs.push_back( pf );
			}
		}

		string nmmp3 = rz::fmt( "{}.mp3", fil::nameNoExt( pf ) );
		if ( !ranges::any_of( *fils, [&]( const fs::path &p ) { return p.filename() == nmmp3; } ) ) continue;

		// //lg::info( "lyric: {}", pf.string() );
		// auto pnew = dtxt / pf.filename();
		//
		// if ( !fs::exists( pnew ) ) {
		// 	dir::ensure( dtxt );
		// 	fs::rename( pf, pnew );
		// }
	}

	if ( !serVo || !serImg ) return; //如果沒有兩種都符合

	for ( auto &p: pimgs ) {
		string pnew = p.parent_path() / rz::fmt( "cg{}", p.filename().string() );
		fs::rename( p, pnew );
	}
}


const std::vector<std::string> lcExts = { ".txt", ".vtt", ".lrc" };

// 找出歌詞類的(跟mp3同名),移到一個txt資料夾
inline void voiceLyric( const fs::path &proot ) {
	using namespace rz::io;
	auto droot = IDir::make( proot );
	auto dtxt = droot->path / "txt";

	auto *fils = &droot->files;

	for ( auto &pf: *fils ) {
		auto ext = pf.extension().string();

		// 檢查是否為歌詞檔案類型
		if ( std::find( lcExts.begin(), lcExts.end(), ext ) == lcExts.end() ) continue;

		auto pnew = dtxt / pf.filename();
		if ( !fs::exists( pnew ) ) {
			dir::ensure( dtxt );
			fs::rename( pf, pnew );
		}
	}
}

// 找出只有一個子資料夾有mp3檔的,移到頂層
inline void voiceOnChildDirMp3( const fs::path &proot ) {
	using namespace rz::io;
	auto droot = IDir::make( proot );


	if ( ranges::any_of( droot->files, []( const fs::path &p ) { return p.extension() == ".mp3"; } ) ) {
		//如果頂層有mp3就不做
		return;
	}


	auto dirMp3 = droot->findDirs( []( const IDir &d ) -> IFindRst {
		if ( ranges::any_of( d.files, []( const fs::path &p ) { return p.extension() == ".mp3"; } ) ) {
			return { .include = true };
		}

		return { .include = false };
	} );

	if ( dirMp3.size() == 1 ) {

		auto idx = 0;
		auto dir = dirMp3[0];

		for ( auto mp3s = dir->find( "*.mp3" ); auto &p: mp3s ) {

			auto nm = p.filename().string();
			auto pn = proot / nm;

			if ( fs::exists( pn ) ) {
				lg::warn( "本要移動的檔案位置已存在: {}", pn.string() );
				continue;
			}

			//lg::info( "move: {} -> {}", p.string(), pn.string() );
			idx++;
			fs::rename( p, pn );
		}

		lg::info( "已移動[ {} ]個檔案: {}", idx, droot->path.filename().string() );
	}

}

}
