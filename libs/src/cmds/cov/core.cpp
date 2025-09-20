#include <rz/idx.h>

#include "./defs.h"
#include "./cov.h"

#include <cmds/idx.h>


using namespace rz;
using namespace rz::io;



inline const auto GL_VQ = 128; //voice quality

inline auto GL_ths_im = 6;
inline auto GL_ths_vo = 6;

#ifdef __APPLE__
inline auto GL_ths_vdo = 1;
#else
inline auto GL_ths_vdo = 3;
#endif

inline const auto ft_arcs = vector{ "zip", "rar" };
inline const auto ft_pics = vector{ "png", "jpg", "webp" };
inline const auto ft_voic = vector{ "wav", "mp3", "m4a", "flac" };
inline const auto ft_vido = vector{ "mp4", "avi", "mpg", "ts", "mkv", "wmv", "mov", "3gp" };

inline const auto vdo_spKeys = vector{ "-smaller-", "?invalid?" };


namespace cmds::cov::core {


inline bool processVideoFile( const fs::path &pathIn, const fs::path &pathOu, const string &stat, const shared_ptr<CovInfo> &ci = nullptr, double *totalRdu = nullptr) {
	try {
		auto cc = cmds::cov::CovCfg::load();

		double szo = io::fil::sizeMB( pathIn );
		lg::info( "[vdo] process [{:.2f}]MB {}", szo, stat );

		auto vi = mda::getVideoInfo( pathIn );

		auto cfg = cc->findCfgVdo( pathOu );
		mda::vdo::opts opt = { .rsz = ops.resize, .txt = stat };
		if ( cfg ) {
			opt.crf = cfg->q;
			opt.mxw = cfg->mxw;
			lg::info( "[vdo] use cfg q[{}] maxW[{}] by {}", cfg->q, cfg->mxw, cfg->desc );
		}

		if ( vi.streams.size() < 2 ) {
			lg::warn( "[vdo] 缺少streams, 資料: {}", vi.raw );
			return false;
		}

		auto pTmp = pathOu.parent_path() / ( pathOu.filename().string() + ".tmp" );

		auto omtime = fs::last_write_time( pathIn );

		auto handleOutputFile = [&omtime]( const fs::path &pathIn, const fs::path &pathOu, const fs::path &pTmp ) {
			if ( pathOu == pathIn ) fs::remove( pathIn );
			fs::rename( pTmp, pathOu );

			try {
				// 獲取原始時間點並轉換為時間戳
				std::time_t orig_time_t;

#if defined(__APPLE__)
				// macOS 處理方式
				auto file_time_since_epoch = omtime.time_since_epoch();
				auto secs = std::chrono::duration_cast<std::chrono::seconds>( file_time_since_epoch );
				orig_time_t = secs.count();
#else
        // Linux 處理方式
        auto sys_time = std::chrono::file_clock::to_sys(omtime);
        orig_time_t = std::chrono::system_clock::to_time_t(sys_time);
#endif

				// 格式化並輸出原始時間
				std::tm *orig_tm = std::localtime( &orig_time_t );

				char orig_time_str[30];
				std::strftime( orig_time_str, sizeof( orig_time_str ), "%Y-%m-%d %H:%M:%S", orig_tm );
				//lg::info( "[vdo] 原始檔案時間: {}", orig_time_str );

				// 設定修改時間
				fs::last_write_time( pathOu, omtime );

				// 獲取並輸出設定後的時間
				auto new_mtime = fs::last_write_time( pathOu );

				std::time_t new_time_t;
#if defined(__APPLE__)
				// macOS 處理方式
				auto new_file_time_since_epoch = new_mtime.time_since_epoch();
				auto new_secs = std::chrono::duration_cast<std::chrono::seconds>( new_file_time_since_epoch );
				new_time_t = new_secs.count();
#else
				// Linux 處理方式
				auto new_sys_time = std::chrono::file_clock::to_sys(new_mtime);
				new_time_t = std::chrono::system_clock::to_time_t(new_sys_time);
#endif

				std::tm *new_tm = std::localtime( &new_time_t );

				char new_time_str[30];
				std::strftime( new_time_str, sizeof( new_time_str ), "%Y-%m-%d %H:%M:%S", new_tm );
				lg::info( "[vdo] done, 檔案時間[{}] {}", new_time_str, pathOu.filename().string() );

				if ( orig_time_t != new_time_t ) throw std::runtime_error( "設定檔案時間失敗.." );

			}
			catch ( const std::exception &ex ) {
				lg::error( "[vdo] 設定檔案時間失敗: {}", ex.what() );
			}
		};
		// 檢查是否符合HEVC格式要求
		bool isRepack = cfg ? ( vi.codec == "hevc" && vi.w <= cfg->mxw )
			            : ( vi.codec == "hevc" && vi.w <= mda::vdo::maxWidth );

		// 根據格式選擇處理方法
		auto ret = isRepack ? mda::vdo::repackMp4( pathIn, pTmp ) : mda::vdo::convH265( pathIn, pTmp, opt );

		lg::info( "[vdo] 檔案格式[{}] w[{}] 執行: {}", vi.codec, vi.w, isRepack ? "Repack" : "convH265" );

		if ( ret ) {
			auto logs = ret.value();
			auto last = logs.back();

			if ( str::contains( logs, "moov atom not found" ) ) {
				lg::warn( "[vdo] 遇到moov atom問題, 略過該檔案: {}", pathIn.filename().string() );
				return false;
			}

			if ( str::contains( logs, "error reading header" ) ) {
				lg::warn( "[vdo] Error Header問題, 更名檔案: {}", pathIn.string() );
				auto newPath = pathIn.parent_path() / rz::fmt( "{}{}.mp4", fil::nameNoExt( pathIn ), vdo_spKeys[1] );
				fs::rename( pathIn, newPath );
				return false;
			}

			for ( auto &s: logs ) lg::warn( "--> {}", s );

			std::string errMsg = isRepack ? "重打包失敗" : "轉檔失敗";
			if ( last.find( "ret:" ) != std::string::npos ) throw std::runtime_error( rz::fmt( "{}, 回傳code[{}]", errMsg, last.substr( 4 ) ) );

			throw std::runtime_error( rz::fmt( "{}, 回傳: {}", errMsg, last ) );
		}

		// HEVC重打包直接完成
		if ( isRepack ) {
			handleOutputFile( pathIn, pathOu, pTmp );

			if ( ci ) {
				ci->vdos.push_back( pathIn.filename().string() );
				ci->save();
			}

			return true;
		}

		// 檢查轉換結果
		double szn = io::fil::sizeMB( pTmp );
		double rdu = szo - szn;

		// 記錄大小變化
		if ( rdu > 0 ) {
			lg::info( "[vdo] done, size[{}]MB->[{}]MB=[-{:.2f}] {}", szo, szn, rdu, pTmp.string() );
		}
		else {
			lg::info( "[vdo] done, size[{}]MB->[{}]MB=[+{:.2f}] {}", szo, szn, -rdu, pTmp.string() );
		}

		// 處理流問題
		if ( vi.streams.size() < 2 ) {
			auto newPath = pathIn.parent_path() / rz::fmt( "{}{}{}.mp4", fil::nameNoExt( pathIn ), vdo_spKeys[1], szn - szo );

			lg::warn( "[vdo] Failed new Stream size[{}] rename[{}]->[{}]", vi.streams.size(), pathIn.filename().string(), newPath.filename().string() );

			fs::rename( pathIn, newPath );
			fs::remove( pTmp );
			return false;
		}

		// 處理輸出檔案大於輸入檔案的情況
		if ( ( szo * 1.1 ) < szn ) {
			auto newFilename = rz::fmt( "{}{}{}.mp4", fil::nameNoExt( pathIn ), vdo_spKeys[0], szn - szo );
			auto newPath = pathIn.parent_path() / newFilename;

			lg::warn( "[vdo] new file bigger then old, rename[{}]->[{}]", pathIn.filename().string(), newFilename );

			fs::rename( pTmp, newPath );

			if ( ci ) {
				ci->vdos.push_back( pathIn.filename().string() );
				ci->vdos.push_back( newFilename );
				ci->save();
			}

			return true;
		}

		handleOutputFile( pathIn, pathOu, pTmp );

		if ( ci ) {
			ci->vdos.push_back( pathIn.filename().string() );
			ci->save();
		}

		if ( rdu > 0 && totalRdu != nullptr ) *totalRdu = *totalRdu + rdu;

		return true;
	}
	catch ( std::exception &ex ) {
		lg::error( "[vdo] 處理失敗: {}", ex.what() );
		return false;
	}
}



inline void onArcFile( const fs::path &pRoot, const fs::path &pArc, arc::IArchive &arc ) {

	auto cc = cmds::cov::CovCfg::load();

	auto cntImg = 0;   //圖片計數
	auto cntImgOk = 0; //已處理數
	auto hasErr = false;

	//------------------------------------------------------------------------
	// 資料夾名稱, 以壓縮檔名為準
	//------------------------------------------------------------------------
	auto nameDir = fil::nameNoExt( pArc );

	auto prev = nameDir;
	nameDir = mda::img::removeNoNeeds( nameDir );
	nameDir = mda::vo::removeNoNeeds( cc->rmQuoteKeys, nameDir );

	//確保不要解壓縮到同目錄
	auto nameDirO = nameDir;
	nameDir = dir::ensureNewName( pRoot / nameDir );

	//確保不要用到已有檔案的資料夾
	if ( nameDir != nameDirO ) {
		auto dirn = pRoot / nameDir;
		auto dirfils = dir::files( pRoot / nameDirO );
		std::erase_if( dirfils, []( const fs::path &p ) { return p.filename().string().front() == '.'; } ); //刪掉所有.開頭的

		if ( dirfils.size() == 0 ) {
			//如果資料夾是空的就用這個
			nameDir = nameDirO;
		}
		else {
			vector<string> tmp;
			for ( const auto &e: *arc.infos ) {
				if ( e.isFil && !str::contains( e.path.filename().string(), cc->excludes, true ) ) tmp.push_back( e.path.string() );
			}

			if ( dirfils.size() == tmp.size() ) {
				lg::info( "[arc] 略過經有相同數量[{}] 的資料夾: {}", dirfils.size(), dirn.string() );
				return;
			}
		}
	}

	lg::info( "[arc] dir: {}", nameDir );


	//------------------------------------------------------------------------
	// 計算要略過的子層資料夾層級
	//------------------------------------------------------------------------
	vector<fs::path> skdirs;
	size_t lvrm = 99;
	for ( const auto &e: *arc.infos ) {

		auto path = e.path;
		auto name = path.filename().string();
		auto exts = path.extension().string();
		auto parent = path.parent_path();

		if ( std::ranges::find( skdirs, parent ) == skdirs.end() ) skdirs.push_back( parent );
		if ( str::contains( name, cc->excludes, true ) ) continue; //需略過的檔案

		if ( auto lv = dir::depth( path ); lvrm > lv ) lvrm = lv; //計算最小階層
		if ( str::contains( exts, ft_pics ) ) cntImg++;           //加總圖片有幾張
		//lg::info( "[info] parent[{}] info: {}", parent.string(), path.string() );
	}
	//for ( auto &p: skdirs ) lg::info( "[資料夾] lv[{}] dir: {}", dir::depth( p ), p.string() );

	std::erase_if( skdirs, [&]( const fs::path &dir ) { return dir::depth( dir ) >= lvrm; } );

	//讓較長的出現在上面
	std::sort( skdirs.begin(), skdirs.end(), []( const fs::path &a, const fs::path &b ) {
		return std::distance( a.begin(), a.end() ) > std::distance( b.begin(), b.end() );
	} );

	//for ( auto &p: skdirs ) lg::info( "[info] skip dirs: {}", p.string() );


	auto repath = [&]( const fs::path &p ) {
		fs::path result = p;
		for ( const auto &skipDir: skdirs ) {
			if ( result.string().find( skipDir.string() ) == 0 ) {
				result = result.lexically_relative( skipDir );
				break;
			}
		}
		return result;
	};

	//------------------------------------------------------------------------
	// extract items
	//------------------------------------------------------------------------
	vector<fs::path> pvos;
	vector<fs::path> dirs;

	TaskPool taskImgs( GL_ths_im );

	TaskPool taskMp3s( GL_ths_vo );

	arc.extr( [&]( const arc::IArcEntry *e ) {

		auto fname = e->path.filename().string();

		//ignore global settings
		if ( str::contains( fname, cc->excludes, true ) ) {
			lg::debug( "[ignore] {}", fname );
			return;
		}

		auto esz = e->size;
		auto exts = e->path.extension().string();
		auto pen = repath( e->path );
		auto pdir = pRoot / nameDir;
		auto pto = pdir / pen;


		if ( !ranges::any_of( dirs, [&]( const fs::path &p ) { return p == pdir; } ) ) {
			dirs.push_back( pdir );
			lg::info( "process: {}", pdir.string() );
		}
		//lg::info( "idx[{}] size[{}] to[{}]", ++idx, e->size, pto );

		auto saveTo = [&]( const fs::path &pathTo ) {
			try {

				lg::info( "[save] size[{:.2f}]MB {}", (double)e->size / 1024 / 1024, fil::nameNoExt( pathTo ) );

				auto bar = code::prog::makeBar( rz::fmt( " {}", pathTo.filename().string() ) );
				e->saveTo( pathTo, [&]( size_t now, size_t total ) {
					auto pc = ( (double)now / (double)total ) * 100;
					bar->setProgress( pc );
				} );
			}
			catch ( std::exception &ex ) {
				auto msg = rz::fmt( "寫檔異常[{}] msg[{}] {}", pen.filename().string(), ex.what(), pathTo.string() );
				lg::error( msg );
			}
		};

		//------------------------------------------------------------------------
		// archives
		//------------------------------------------------------------------------
		if ( str::contains( exts, ft_arcs ) ) {

			lg::info( "------------------------------------" );
			lg::info( "archive: {}", fname );
			lg::info( "------------------------------------" );

			//onArchiveFile( pRoot, pRoot / pathto.stem(), ar );
			auto namePto = fil::nameNoExt( pto );
			auto nameArc = fil::nameNoExt( pArc );

			//預設是用資料夾名稱，如果壓縮檔有日文字體就是用壓縮檔名(nested zip)
			auto pcdir = pRoot / namePto;
			if ( str::hasJP( nameArc ) && !str::hasJP( namePto ) ) pcdir = pRoot / nameArc;

			dir::ensure( pcdir );

			//lg::info( "archive arc:\n dir[{}] hasJP[{}]", pArc.stem().string(), str::hasJP( pArc.stem().string() ) );
			//lg::info( "archive pto:\n dir[{}] hasJP[{}]", pathto.stem().string(), str::hasJP( pathto.stem().string() ) );

			//auto pcto = pRoot / pathto.stem();
			//lg::info( "[archive]\n pRoot[{}]\n pArc[{}]\n pathto[{}]\n fname[{}]\n pchild[{}]", pRoot, pArc, pathto, fname, pcdir );

			shared_ptr<fs::path> ptmp;

			if ( str::contains( fname, ".part" ) && str::contains( fname, ".rar" ) ) {

				ptmp = make_shared<fs::path>( pto );
			}
			else {

				ptmp = fil::tmp( fname, true );
				lg::info( "save tmp archive to: {}", ptmp->string() );

				auto bar = code::prog::makeBar( "to tmp" );
				e->saveTo( *ptmp, [&]( size_t now, size_t total ) {
					auto pc = ( (double)now / (double)total ) * 100;
					bar->setProgress( pc );
				} );

				lg::info( "extract tmp to: {}", pcdir.string() );
			}

			try {
				arc::extract( *ptmp, [&]( arc::IArchive &ar ) {
					onArcFile( pRoot, pcdir, ar );
				} );
			}
			catch ( std::exception &ex ) {

				auto bar = code::prog::makeBar( "to file" );
				e->saveTo( pto, [&]( size_t now, size_t total ) {
					auto pc = ( (double)now / (double)total ) * 100;
					bar->setProgress( pc );
				} );
				lg::error( "[arc:file] {} failed, {}", nameArc, ex.what() );

				hasErr = true;
			}



			lg::info( "------------------------------------" );
			return;
		}

		dir::ensure( pto );


		//------------------------------------------------------------------------
		// pic
		//------------------------------------------------------------------------
		if ( str::contains( exts, ft_pics ) ) {
			pto.replace_extension( ".webp" );
			auto data = e->read();

			taskImgs.enqueue( [=,&cntImgOk]() {
				try {
					auto cfg = cc->findConfigImg( pto );

					auto d = data;
					auto img = mda::img::resize( *d, cfg->w, cfg->h, { .increase = ops.forceIncr || cfg->forceScale } );
					mda::img::saveWebp( img, pto, cfg->q );
					cntImgOk++;
				}
				catch ( std::exception &ex ) {

					auto msg = rz::fmt( "[img] failed, {}", ex.what() );
					throw std::runtime_error( msg );
				}
			} );
			return;
		}

		//------------------------------------------------------------------------
		// voice
		//------------------------------------------------------------------------
		if ( str::contains( exts, ft_voic ) ) {
			try {
				auto pmp3 = pto.parent_path() / rz::fmt( "{}.mp3", fil::nameNoExt( pto ) );

				auto exist = ranges::any_of( pvos, [&]( const fs::path &p ) { return p == pmp3; } );
				if ( exist ) {
					lg::info( "[arc:vo] 略過相同檔案, {} ({})", pmp3.filename().string(), e->path.filename().string() );
					return;
				}

				pvos.push_back( pmp3 );

				auto ptmp = fil::tmp( pto.filename(), true );

				auto data = e->read();
				lg::info( "[arc:vo] size[{:.2f}]MB save to tmp, file[{}]", ( (double)data->size() / 1024 / 1024 ), fil::nameNoExt( pto ) );

				fil::save( *ptmp, data ); //寫入tmp檔案

				auto state = rz::fmt( "{}|{}", taskMp3s.size(), pto.filename().string() );

				taskMp3s.enqueue( [=,&pvos]() {
					mda::vo::convMP3( *ptmp, pmp3, { .kbps = GL_VQ, .txt = state } );
				} );
			}
			catch ( std::exception &ex ) {
				lg::error( "[arc:vo] handle failed, {}", ex.what() );

				throw std::runtime_error( rz::fmt( "[arc:vo] 異常, {}", ex.what() ) );
			}

			return;
		}

		//------------------------------------------------------------------------
		// video
		//------------------------------------------------------------------------
		if ( !ops.ignoreVideo && str::contains( exts, ft_vido ) ) {
			try {
				auto pout = pto.parent_path() / rz::fmt( "{}.mp4", fil::nameNoExt( pto ) );

				// 檢查是否已處理過相同檔案
				auto exist = ranges::any_of( pvos, [&]( const fs::path &p ) { return p == pout; } );
				if ( exist ) {
					lg::info( "[arc:vdo] 略過相同檔案, {} ({})", pout.filename().string(), e->path.filename().string() );
					return;
				}

				if ( fs::exists( pout ) ) {
					auto sz = fs::file_size( pout );
					lg::warn( "[arc:vdo] override exists file size: {:.2f}MB", sz / 1024 / 1024 );
				}

				pvos.push_back( pout );

				if ( esz > ( 1024 * 1024 ) ) {
					lg::info( "[arc:vdo] size[{:.2f}MB]", esz / 1024 / 1024 );
				}
				else {
					lg::info( "[arc:vdo] size[{:.2f}KB]", esz / 1024 );
				}

				saveTo( pout );

				// 轉檔邏輯
				// auto ptmp = fil::tmp( pto.filename(), true );
				// saveTo( *ptmp );
				// auto stat = pto.filename().string();
				// if ( !processVideoFile( *ptmp, pout, stat, nullptr ) ) {
				// 	throw std::runtime_error( rz::fmt( "[arc:vdo] 處理失敗: {}", pto.filename().string() ) );
				// }
			}
			catch ( std::exception &ex ) {
				lg::error( "[arc:vdo] handle failed, {}", ex.what() );
				throw std::runtime_error( rz::fmt( "[arc:vdo] 異常, {}", ex.what() ) );
			}
			return;
		}

		//------------------------------------------------------------------------
		// others
		//------------------------------------------------------------------------
		saveTo( pto );

	} );


	taskImgs.onError( [&]( const std::exception &ex ) {
		lg::error( "[arc:img] cov error: {}", ex.what() );
	} );
	taskMp3s.onError( [&]( const std::exception &ex ) {
		lg::error( "[arc:mp3] cov error: {}", ex.what() );
	} );

	auto bar = code::prog::makeBar( rz::fmt( " img({})", taskImgs.size() ), { .max = taskImgs.size() } );
	taskImgs.runWaitAll( [&]( int idx, int ) { bar->setProgress( idx ); } );


	taskMp3s.runWaitAll();

	if ( pvos.size() > 0 ) {
		lg::info( "[arc:vo] gain: {}", str::join( pvos, ",", []( const fs::path &p ) { return p.filename().string(); } ) );

		auto csiz = pvos.size();
		auto cmax = csiz * 100;
		auto bar = code::prog::makeBar( rz::fmt( "gain:1/{}", csiz ), { .max = cmax } );
		mda::vo::gainAlbum( pvos, [&]( int idx, int percent, unsigned long ) {

			auto now = idx <= 0 ? percent : ( percent + ( ( idx - 1 ) * 100 ) );
			bar->setProgress( now );
			bar->setPrefix( rz::fmt( "gain:{}/{}", idx, csiz ) );

		} );
	}

	if ( cntImg != cntImgOk ) {
		auto msg = rz::fmt( "[arc:img] conv failed: [{}] / [{}]", cntImgOk, cntImg );
		throw new std::runtime_error( msg );
	}

	if ( hasErr ) throw new std::runtime_error( "[arc] has internal error..." );
}


}



namespace cmds::cov::core {

inline void doVoiceDirs( const IDir *rootDir ) {

	lg::info( "------------------ process voice move ------------------" );
	//------------------------------------------------------------------------
	// 找出符合命名的資料夾 [xxxx] xxxxxx [RJxxxx] 2
	//------------------------------------------------------------------------
	auto dirs = rootDir->findDirs( [&]( const IDir &dir ) -> IFindRst {

		if ( mda::vo::isMatchDirName( dir.path ) ) return { .include = true };

		return { .toDeep = true, .include = false };
	} );

	lg::info( "dirs: {}", dirs.size() );

	for ( auto &d: dirs ) {

		if ( !exists( d->path ) ) {
			lg::warn( "[vo:move] 資料夾路徑已不存在: {}", d->path.string() );
			continue;
		}

		try {
			//移mp3至頂層
			mv::voiceOnChildDirMp3( d->path );
			//將頂層與mp3相同檔, 移動至txt資料夾 (txt或img類)
			mv::voiceLyric( d->path );
			//將與mp3混在一起的img加上cg
			mv::voiceRenamePicIfNameClose( d->path );
		}
		catch ( exception &ex ) {
			lg::error( "[vo:move] 發生異常, {}", ex.what() );
		}
	}
}



inline void doImageDirs( const fs::path &rootPath ) {

	lg::info( "------------------ process move images ------------------" );
	lg::info( "excludes: {}", ops.cc->excludes );
	lg::info( "---------------------------------------------------------" );
	IDir::make( rootPath ).get()->findDirs( []( const IDir &dir ) -> IFindRst {
		for ( auto &fil: dir.files ) {

			auto name = fil.filename().string();
			if ( str::contains( name, ops.cc->excludes, true ) ) fs::remove( fil );
		}
		return { .include = true };
	} );



	auto dirs = IDir::make( rootPath ).get()->findDirs( []( const IDir &dir ) -> IFindRst {
		for ( auto &fil: dir.files ) {
			if ( str::contains( fil.extension().string(), ft_pics ) ) return { .include = true };
		}
		return { .include = false };
	} );

	//將folder後面有 (1)的,比對看看有沒有同名資料夾
	for ( auto &d: dirs ) {
		auto name = d->path.filename().string();
		if ( !str::rgx::has( name, R"( \([0-9]+\))" ) ) continue;

		auto nnam = str::rgx::replace( name, R"( \([0-9]+\))", "" );
		auto pdir = d->path.parent_path();
		auto np = pdir / nnam;

		if ( fs::exists( np ) ) continue;

		lg::info( "[dirs] 更名: {} -> {}", name, nnam );
		fs::rename( d->path, np );
	}


	// remove empty dirs
	IDir::make( rootPath ).get()->findDirs( []( const IDir &dir ) -> IFindRst {

		if ( dir.infos.deep != 0 || dir.infos.files >= 2 ) return { .include = false };
		if ( dir.files.empty() || dir.files.front().filename() == ".cov" ) {

			lg::info( "[img:dir:del] dir[deep:{}|files:{}][dirs:{}|files:{}] {}", dir.infos.deep, dir.infos.files, dir.dirs.size(), dir.files.size(), dir.path.string() );
			dir.del();
		}

		return { .include = true };
	} );
}


}
