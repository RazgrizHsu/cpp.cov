#include <rz/idx.h>


#include "./core.cpp"


namespace cmds::cov {

Opts ops{
	.ignoreVideo = false,
};

void doArchives( const IDir *rootDir ) {

	auto parcs = rootDir->find( "*.rar", "*.zip" ); /* 刪除多餘的rar parts */
	removeRarPartsAll( parcs );
	lg::info( "[arc] Founds archives [ {} ]", parcs.size() );

	for ( auto &parc: parcs ) {

		lg::info( "================== archive ==================" );
		lg::info( "[arc:file] {}", parc.string() );

		vector<fs::path> pRars;

		bool ok = true;

		auto fils = arc::extract( parc, [&]( arc::IArchive &arc ) {

			try { core::onArcFile( rootDir->path, parc, arc ); }
			catch ( exception &ex ) {
				lg::error( "[arc:file] 異常, {}, file: {}", ex.what(), parc.string() );
				ok = false;
			}

		} );

		if ( ok ) for ( auto &p: *fils ) fs::remove( p );

	}
}


void doVoices( const IDir *rootDir ) {

	auto cc = cmds::cov::CovCfg::load();
	const auto proot = rootDir->path;

	//------------------------------------------------------------------------
	// ignore & preprocess
	//------------------------------------------------------------------------
	lg::info( "------------------ process voice ------------------" );
	auto dirs = rootDir->findDirs( []( const IDir &dir ) -> IFindRst {

		// 如果是分類目錄,不加入
		if ( dir.infos.deep > 0 && dir.infos.files > 0 && dir.files.size() == 0 ) return { .include = false };

		return {};
	} );


	for ( auto &dir: dirs ) {
		if ( dir->infos.files != 0 ) continue;
		if ( dir->path == rootDir->path ) continue;
		lg::info( "[vo:del] dir[deep:{}|files:{}][dirs:{}|files:{}] {}", dir->infos.deep, dir->infos.files, dir->dirs.size(), dir->files.size(), dir->path.string() );
		dir->del();
	}

	auto dels = rootDir->find( "*.tmp" );
	for ( auto &fil: dels ) {
		lg::info( "[del] tmp: {}", fil.string() );
		fs::remove_all( fil );
	}


	//------------------------------------------------------------------------
	// 找出符合命名的資料夾 [xxxx] xxxxxx [RJxxxx] 2
	//------------------------------------------------------------------------
	dirs = rootDir->findDirs( [&]( const IDir &dir ) -> IFindRst {

		if ( mda::vo::isMatchDirName( dir.path ) ) return { .include = true };

		return { .toDeep = true, .include = false };
	} );

	//------------------------------------------------------------------------
	// 更名
	//------------------------------------------------------------------------
	for ( auto &dir: dirs ) {
		auto name = dir->path.filename().string();
		auto nnam = mda::vo::removeNoNeeds( cc->rmQuoteKeys, name );
		if ( name != nnam ) {
			auto newPath = dir->rename( nnam, true );
			*dir = IDir::make( newPath );
			lg::debug( "資料夾更名:\n src:{}\n dst:{}", name, nnam );
		}
	}

	//------------------------------------------------------------------------
	// 找出.cov
	//------------------------------------------------------------------------
	map<IDir *, vector<fs::path>> maps;

	auto cTotal = 0;
	auto cProce = 0;

	for ( auto &dir: dirs ) {

		if ( !exists( dir->path ) ) {
			lg::warn( "[vo] 資料夾路徑已不存在: {}", dir->path.string() );
			continue;
		}

		auto ci = CovInfo::get( dir->path );
		if ( ci->vo ) continue; //已轉檔過就略過


		auto fils = dir->find( "*.mp3", "*.wav", "*.flac", "*.m4a" );
		if ( fils.size() <= 0 ) continue;

		vector<fs::path> toFiles;
		for ( auto &fil: fils ) {
			toFiles.push_back( fil );
			cTotal++;
		}

		maps.insert( { dir.get(), toFiles } );
	}

	for ( auto pair: maps ) {

		auto dir = pair.first;
		auto fils = pair.second;

		auto ci = CovInfo::get( dir->path );
		if ( ci->vo ) continue; //已轉檔過就略過

		auto p = str::replaceAll( dir->path.string(), rootDir->path.string(), "" );
		if ( fils.size() > 0 ) {
			lg::info( "[vo] === dir ===> {}", p );
			//lg::info( "[scan voice] infos[{:2}|{:2}|{:2}|{:2}] {}", dir->infos.deep, dir->infos.files, dir->dirs.size(), dir->files.size(), p );
		}

		auto hasError = false;
		vector<fs::path> gainFils;
		mutex gainFilsMtx;

		TaskPool pool( GL_ths_vo );

		for ( auto &fil: fils ) {

			auto state = rz::fmt( "{}/{}", cProce++, cTotal );
			auto ext = fil.extension().string();

			auto cov = true, covRe = false;
			if ( ext == ".mp3" ) {
				auto rate = mda::getBitRate( fil );
				if ( rate <= ( GL_VQ + 3 ) ) cov = false;
				else {
					lg::info( "[vo] bitrate[{}] {}", rate, fs::relative( fil, proot ).string() );
					covRe = true;
				}
			}

			if ( !cov ) continue;

			if ( !exists( fil ) ) {
				lg::warn( "[vo] 檔案路徑已不存在: {}", fil.string() );
				continue;
			}

			pool.enqueue( [=,&gainFils,&gainFilsMtx,&hasError]() {

				lg::info( "[vo] ({}) start: {}", state, fil.filename().string() );
				auto ptmp = fil.parent_path() / rz::fmt( "{}.tmp", fil::nameNoExt( fil ) );
				auto pmp3 = fil.parent_path() / rz::fmt( "{}.mp3", fil::nameNoExt( fil ) );

				try {
					mda::vo::convMP3( fil, ptmp, { .kbps = GL_VQ, .txt = state } );

					fs::rename( ptmp, pmp3 );

					if ( covRe ) {
						//auto bitrate = mda::getBitRate( pmp3 );
						lg::info( "[vo] ReConv: {}", rz::str::limitPath( fs::relative( pmp3, proot ) ) );
					}
					else {
						fs::remove( fil ); //如果跟原副檔名不同
					}

					{
						lock_guard<mutex> lock( gainFilsMtx );
						gainFils.push_back( pmp3 );
					}
				}
				catch ( std::exception &ex ) {
					lg::error( "[vo] cov failed, {}", ex.what() );
					hasError = true;
				}
			} );
		}

		pool.runWaitAll();

		//音量
		auto csiz = fils.size();
		auto cmax = csiz * 100;
		auto bar = code::prog::makeBar( rz::fmt( " gain:1/{}", csiz ), { cmax } );
		mda::vo::gainAlbum( gainFils, [&]( int idx, int percent, unsigned long ) {

			auto now = idx <= 0 ? percent : ( percent + ( ( idx - 1 ) * 100 ) );
			bar->setProgress( now );
			bar->setPrefix( rz::fmt( " gain:{}/{}", idx, csiz ) );

		} );

		if ( hasError ) continue; //如果有問題不要儲存
		ci->vo = 1;
		ci->save();

	}

}


void doImages( const IDir *rootDir ) {
	//========================================================================
	// 處理圖片
	//========================================================================
	auto cc = cmds::cov::CovCfg::load();
	lg::info( "------------------ process pics ------------------" );
	auto dirs = rootDir->findDirs( []( const IDir &dir ) -> IFindRst {

		// 如果是分類目錄,不加入
		if ( dir.infos.deep > 0 && dir.infos.files > 0 && dir.files.size() == 0 ) return { .include = false };

		return {};
	} );

	dirs = rootDir->findDirs( []( const IDir &dir ) -> IFindRst {

		for ( auto &fil: dir.files ) {
			if ( str::contains( fil.extension().string(), ft_pics ) ) return { .include = true };
		}

		return { .include = false };
	} );

	//------------------------------------------------------------------------
	// 更名
	//------------------------------------------------------------------------
	for ( auto &dir: dirs ) {
		auto name = dir->path.filename().string();
		auto nnam = mda::img::removeNoNeeds( name );
		if ( name != nnam ) {
			auto newPath = dir->rename( nnam, true );
			*dir = IDir::make( newPath );
			lg::debug( "資料夾更名:\n src:{}\n dst:{}", name, nnam );
		}
	}

	auto idx = 0;
	auto siz = dirs.size();
	for ( auto &dir: dirs ) {

		idx++;
		if ( !fs::exists( dir->path ) ) {
			lg::warn( "[pic] 資料夾路徑已不存在: {}", dir->path.string() );
			continue;
		}

		auto ci = CovInfo::get( dir->path );
		if ( !ops.forceIncr && ci->img ) continue; //已轉檔過就略過

		auto cfg = cc->findConfigImg( dir->path );
		if ( !cfg->enable ) continue; //設定為不轉換就略過

		lg::info( "[pic][{}/{}] scan [deep:{}|files:{}][dirs:{}|files:{}]\n dir: {}", idx, siz, dir->infos.deep, dir->infos.files, dir->dirs.size(), dir->files.size(), dir->path.string() );

		auto hasError = false;
		lg::info( "settings: {}x{}, q={}, scale={}, enable={}, {}", cfg->w, cfg->h, cfg->q, cfg->forceScale, cfg->enable, dir->path.string() );

		TaskPool tasks( GL_ths_im );


		for ( auto &fil: dir->files ) {
			auto ext = fil.extension().string();
			if ( !str::contains( ext, ft_pics ) ) continue;

			auto isRe = str::contains( fil.filename().extension().string(), "webp" );

			auto pto = fil.parent_path() / rz::fmt( "{}.webp", fil::nameNoExt( fil ) );

			//lg::info( "[img] process: {}", fil.filename().string() );

			if ( !isRe && exists( pto ) ) remove_all( pto );

			if ( !ops.forceIncr && !cfg->forceScale && isRe && fil::sizeKB( fil ) <= 400 ) continue;


			auto data = make_shared<vector<uint8_t>>( fil::read( fil ) );

			tasks.enqueue( [=,&hasError]() {

				try {
					lg::info( "[img] {} {}", isRe ? "RE=>" : "", fil.filename().string() );
					auto img = mda::img::resize( *data, cfg->w, cfg->h, { .increase = ops.forceIncr || cfg->forceScale } );
					mda::img::saveWebp( img, pto, isRe ? cfg->q - 2 : cfg->q );
					if ( !isRe ) fs::remove( fil );

				}
				catch ( std::exception &ex ) {
					lg::error( "[img] cov failed, {}, fil: {}", ex.what(), fil.string() );
					hasError = true;
				}
			} );
		}

		tasks.onError( [&]( const std::exception &ex ) {
			lg::error( "[img] cov error: {}", ex.what() );
			hasError = true;
		} );

		auto bar = code::prog::makeBar( rz::fmt( " cov({})", tasks.size() ), {tasks.size()} );
		tasks.runWaitAll( [&]( int idx, int ) { bar->setProgress( idx ); } );

		if ( hasError ) continue; //如果有問題不要儲存
		ci->img = 1;
		ci->save( dir->path );
	}
}


void doVideos( const IDir *rootDir ) {
	if ( ops.ignoreVideo ) return;

	auto cc = cmds::cov::CovCfg::load();

	lg::info( "------------------ process videos ------------------" );
	auto dirs = rootDir->findDirs( []( const IDir &dir ) -> IFindRst {

		if ( dir.infos.deep > 0 && dir.infos.files > 0 && dir.files.size() == 0 ) return { .include = false };

		return {};
	} );

	dirs = rootDir->findDirs( []( const IDir &dir ) -> IFindRst {

		for ( auto &fil: dir.files ) if ( str::contains( fil.extension().string(), ft_vido ) ) return { .include = true };
		return { .include = false };
	} );

	//------------------------------------------------------------------------
	map<fs::path, shared_ptr<CovInfo>> vdoFiles;

	for ( auto &dir: dirs ) {
		if ( !fs::exists( dir->path ) ) {
			lg::warn( "[vdo] 資料夾路徑已不存在: {}", dir->path.string() );
			continue;
		}

		auto ci = CovInfo::get( dir->path );
		auto *dones = &ci->vdos;
		//lg::info( "[vdo]scan [deep:{}|files:{}][dirs:{}|files:{}]\n dir: {}", dir->infos.deep, dir->infos.files, dir->dirs.size(), dir->files.size(), dir->path.string() );

		for ( auto &p: dir->files ) {
			auto nmf = p.filename().string();
			auto ext = p.extension().string();

			lg::info( "[vdo] scan: nmf[{}] ext[{}]", nmf, ext );

			if ( !str::contains( ext, ft_vido, true ) ) continue;
			if ( str::contains( nmf, *dones ) ) continue;
			if ( str::contains( nmf, vdo_spKeys ) ) {

				lg::debug( "[vdo] 略過檔案: {}", p.string() );
				continue;
			}

			auto nmfsmall = rz::fmt( "{}{}", fil::nameNoExt( p ), vdo_spKeys[0] );
			if ( str::contains( dir->files, nmfsmall ) ) {
				lg::debug( "[vdo] 略過已存在轉過的額外檔案, {}", p.string() );
				continue;
			}

			vdoFiles.insert( { p, ci } );
		}
	}
	TaskPool taskVdos( GL_ths_vdo );

	int idx = 0;
	double trdu = 0;

	for ( auto &pair: vdoFiles ) {
		fs::path p = pair.first;
		string pstr = p.string();
		shared_ptr<CovInfo> ci = pair.second;

		auto pmp4 = p.parent_path() / rz::fmt( "{}.mp4", fil::nameNoExt( p ) );

		taskVdos.enqueue( [=, &vdoFiles, &idx, &trdu]() {
			idx++;

			auto stat = fmt::format( "[{}/{}] {}", idx, vdoFiles.size(), p.filename().string() );

			auto ok = core::processVideoFile( p, p, stat, ci, &trdu );
			if ( !ok ) throw std::runtime_error( rz::fmt( "[arc:vdo] 處理失敗: {}", p.filename().string() ) );

		} );
	}
	taskVdos.onError( [&]( const std::exception &ex ) {
		lg::error( "[vdo] conv error: {}", ex.what() );
	} );

	taskVdos.runWaitAll();

	lg::info( "[vdo] total reduce size [{:.2f}]MB", trdu );
}



}


namespace cmds::cov {


void startBy( const fs::path &path, bool logFile ) {

	cmds::run( path, [&]() {

		if ( !fs::exists( path ) ) {
			lg::info( "path wrong: {}", path.string() );
			return;
		}

		auto ci = CovCfg::load();

		ops.cc = ci;


		lg::info( "========================================================================" );
		lg::info( "💖 conv Dir: {}", path.string() );
		lg::info( "💖 ignore video[{}]", ops.ignoreVideo );
		lg::info( "------------------------------------------------------------------------" );

		try { doArchives( IDir::make( path ).get() ); }
		catch ( exception &ex ) { lg::error( "[start] Archives異常, {}", ex.what() ); }

		try { doVoices( IDir::make( path ).get() ); }
		catch ( exception &ex ) { lg::error( "[start] Voices異常, {}", ex.what() ); }

		try { doImages( IDir::make( path ).get() ); }
		catch ( exception &ex ) { lg::error( "[start] Images異常, {}", ex.what() ); }

		try { doVideos( IDir::make( path ).get() ); }
		catch ( exception &ex ) { lg::error( "[start] Videos異常, {}", ex.what() ); }

		//------------------------------------
		// renames
		//------------------------------------
		try { core::doImageDirs( path ); }
		catch ( exception &ex ) { lg::error( "[start] mv:img異常, {}", ex.what() ); }

		try { core::doVoiceDirs( IDir::make( path ).get() ); }
		catch ( exception &ex ) { lg::error( "[start] mv:vo異常, {}", ex.what() ); }


		lg::info( "------------------------------------------------------------------------" );
		lg::info( "done" );
		lg::info( "========================================================================" );


	}, { .logToFile = logFile } );
}


}
