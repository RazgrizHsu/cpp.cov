#include <iostream>
#include <rz/idx.h>
#include <cmds/idx.h>

#include <unordered_set>

using namespace std;

#include <signal.h>
#include <unistd.h>
#include <sys/select.h>

void crash_handler( int sig ) {

	fprintf( stderr, "\nCrash! Signal %d received. PID: %d\n", sig, getpid() );
	fprintf( stderr, "Press Ctrl-D to exit, or attach debugger and set wait_for_debugger=false\n" );

	volatile bool wait_for_debugger = true;

	// 檢查 stdin 是否是終端
	bool is_tty = isatty( STDIN_FILENO );

	fd_set fds;
	struct timeval tv;

	while ( wait_for_debugger ) {
		// 只在終端模式下檢查輸入
		if ( is_tty ) {
			FD_ZERO( &fds );
			FD_SET( STDIN_FILENO, &fds );
			tv.tv_sec = 0;
			tv.tv_usec = 100000; // 100ms

			int ret = select( STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv );
			if ( ret > 0 && FD_ISSET( STDIN_FILENO, &fds ) ) {
				char buf[1];
				ssize_t n = read( STDIN_FILENO, buf, sizeof(buf) );
				if ( n == 0 ) { // EOF (Ctrl-D)
					fprintf( stderr, "Received EOF, exiting...\n" );
					exit( 1 );
				}
			}
		}
		else {
			// 非終端模式，只等待
			sleep( 1 );
		}
	}

	// when debugger attached, can tune `wait_for_debugger = false` to continue
}


int main( int argc, char *argv[] ) {

	signal( SIGSEGV, crash_handler );
	signal( SIGABRT, crash_handler );

	cli::Ap ap( "cov" );


	//------------------------------------------------------------------------
	// 合併files
	//------------------------------------------------------------------------
	ap.cmd( "single", "single from dirs" )
	  .arg<string>( "path", "convert path if not set use current", true )
	  .handle( []( const cli::Args &args ) {

		  auto path = args.get<fs::path>( "path", fs::current_path() );

		  cmds::cov::mv::extractSingleFiles( path );
	  } );

	//------------------------------------------------------------------------
	// move files
	//------------------------------------------------------------------------
	ap.cmd( "mov", "move files" )
	  .arg<string>( "path", "convert path if not set use current", true )
	  .handle( []( const cli::Args &args ) {

		  auto path = args.get<fs::path>( "path", fs::current_path() );

		  cmds::cov::mv::moveArts( path );

	  } );

	//------------------------------------------------------------------------
	// move files
	//------------------------------------------------------------------------
	ap.cmd( "ffm", "ffmpeg" )
	  .arg<string>( "url", "convert path if not set use current", false )
	  .arg<string>( "fileName", "convert path if not set use current", false )
	  .handle( []( const cli::Args &args ) {

		  auto dir = fs::current_path();
		  auto url = args.get<string>( "url" );
		  auto nmf = args.get<string>( "fileName" );

		  if ( !nmf.ends_with( "mp4" ) ) nmf += ".mp4";

		  auto pto = dir / nmf;

		  lg::info( "to => {}", rz::str::limit( pto.string(), 50 ) );

		  if ( fs::exists( pto ) ) {
			  lg::warn( "target already exists: {}", nmf );
			  return;
		  }

		  rz::mda::vdo::download( url, pto, { .txt = "" } );

	  } );

	const string CovFFMs = ".cov.ffms";
	ap.cmd( "ffms", "ffmpeg" )
	  .handle( [&]( const cli::Args &args ) {
		  auto dir = fs::current_path();
		  auto cfs = rz::io::fil::readSafe<string>( dir / CovFFMs );
		  string line;
		  lg::info( "輸入url, 空行開始下載, 輸入continue或c載入快取..." );
		  std::vector<std::pair<string, string>> tasks;
		  std::unordered_set<string> cachedUrls;
		  bool loadedFromCache = false;

		  // 
		  while ( true ) {
			  std::getline( std::cin, line );
			  if ( std::cin.fail() && !std::cin.eof() ) {
				  std::cin.clear();
				  continue;
			  }
			  line.erase( std::remove( line.begin(), line.end(), '\r' ), line.end() );

			  // 處理"continue"指令 - 從快取檔案加載任務
			  if ( line == "continue" || line == "c" ) {
				  if ( cfs.empty() ) {
					  lg::warn( "快取檔案為空或不存在: {}", dir / CovFFMs );
					  continue;
				  }

				  lg::info( "讀取到[{}]行資料..", cfs.size() );

				  std::vector<std::pair<string, string>> cachedTasks;
				  for ( const auto &cache_line: cfs ) {
					  string line_copy = cache_line;
					  line_copy.erase( std::remove( line_copy.begin(), line_copy.end(), '\r' ), line_copy.end() );
					  if ( line_copy.empty() ) continue;

					  if ( line_copy.find( "cov ffm" ) == 0 ) line_copy = line_copy.substr( 7 );
					  std::istringstream iss( line_copy );
					  string url, nmf;

					  if ( iss >> std::quoted( url ) >> std::quoted( nmf ) ) {
						  string fnm = nmf;
						  if ( !fnm.ends_with( ".mp4" ) ) fnm += ".mp4";
						  auto pto = dir / fnm;

						  if ( fs::exists( pto ) ) {
							  lg::info( "跳過已存在的檔案: {}", fnm );
							  continue;
						  }

						  if ( cachedUrls.find( url ) != cachedUrls.end() ) {
							  lg::info( "跳過重複的URL: {}", url );
							  continue;
						  }

						  cachedTasks.emplace_back( url, fnm );
						  cachedUrls.insert( url );
					  }
				  }

				  // 顯示讀取到的任務並請求確認
				  if ( cachedTasks.empty() ) {
					  lg::warn( "快取檔案中沒有找到有效的任務。" );
					  continue;
				  }

				  lg::info( "在快取檔案中找到 {} 個任務:", cachedTasks.size() );
				  for ( size_t i = 0; i < cachedTasks.size(); ++i ) {
					  lg::info( "[{}] {} -> {}", i + 1, cachedTasks[i].first, cachedTasks[i].second );
				  }

				  lg::info( "你要執行這 {} 個任務嗎？(y/n)", cachedTasks.size() );
				  string confirm;
				  std::getline( std::cin, confirm );

				  if ( confirm == "y" || confirm == "Y" || confirm == "yes" || confirm == "Yes" ) {
					  tasks.insert( tasks.end(), cachedTasks.begin(), cachedTasks.end() );
					  lg::info( "從快取檔案添加了 {} 個任務。", cachedTasks.size() );
					  loadedFromCache = true; // 標記為從快取加載
					  break;                  // 確認後退出輸入循環，開始下載
				  }

				  lg::info( "取消從快取檔案加載。" );
			  }
			  else if ( line.empty() ) {
				  break; // 空行，退出輸入循環
			  }
			  else {
				  // 原有的輸入處理邏輯
				  if ( line.find( "cov ffm" ) == 0 ) line = line.substr( 7 ); // 移除前綴 "cov ffm" 如果存在
				  std::istringstream iss( line );
				  string url, nmf;
				  if ( iss >> std::quoted( url ) >> std::quoted( nmf ) ) {
					  if ( cachedUrls.find( url ) != cachedUrls.end() ) {
						  lg::warn( "跳過，已在快取中: {}", url );
						  continue;
					  }
					  string fnm = nmf;
					  if ( !fnm.ends_with( ".mp4" ) ) fnm += ".mp4";
					  auto pto = dir / fnm;
					  if ( fs::exists( pto ) ) {
						  lg::warn( "跳過已存在的檔案: {}", fnm );
						  continue;
					  }
					  tasks.emplace_back( url, fnm );
					  cachedUrls.insert( url );
					  lg::info( "添加任務[{}]: {}", tasks.size(), nmf );
				  }
				  else {
					  lg::warn( R"(輸入格式無效。預期格式: "url" "檔案名")" );
				  }
			  }
		  }

		  if ( tasks.empty() ) {
			  lg::info( "沒有任務需要執行。" );
			  return;
		  }

		  // 如果不是從快取載入的任務，則將任務資料儲存到快取檔案
		  if ( !loadedFromCache && !tasks.empty() ) {
			  try {
				  std::vector<string> nvec;
				  // 保留原有的快取資料
				  for ( const auto &line: cfs ) nvec.push_back( line );
				  for ( const auto &[url, fnm]: tasks ) nvec.push_back( fmt::format( "\"{}\" \"{}\"", url, fnm ) );

				  // 寫入檔案
				  rz::io::fil::save( dir / CovFFMs, nvec );
				  lg::info( "已將 {} 個新任務保存到快取檔案。", tasks.size() );
			  }
			  catch ( const std::exception &ex ) {
				  lg::error( "保存任務到快取檔案時發生錯誤: {}", ex.what() );
			  }
		  }

		  TaskPool tsk( 3 );
		  const int ctsk = tasks.size();
		  const auto durTimeout = std::chrono::hours( 2 ); // 設定2小時超時

		  lg::info( "開始下載 {} 個檔案到 {}", tasks.size(), dir.string() );

		  for ( int idx = 0; idx < ctsk; ++idx ) {
			  const auto &[url, nmf] = tasks[idx];
			  tsk.enqueue( [&dir, url, nmf, idx, ctsk, durTimeout]() {
				  string fnm = nmf;
				  if ( !fnm.ends_with( ".mp4" ) ) fnm += ".mp4";
				  auto pto = dir / fnm;
				  if ( fs::exists( pto ) ) {
					  lg::warn( "跳過已存在的檔案: {}", fnm );
					  return;
				  }

				  auto txt = fmt::format( "[{}/{}] {}", idx + 1, ctsk, rz::str::limit( nmf, 30 ) );

				  lg::info( "{} 下載至 => {}", txt, rz::str::limit( pto.string(), 50 ) );
				  try {
					  auto rst = rz::mda::vdo::download( url, pto, { .txt = txt } );
					  if ( !rst ) lg::info( "完成: {}", rz::str::limit( fnm, 30 ) );
					  else {
					  }
				  }
				  catch ( const std::exception &ex ) {
					  lg::error( "下載 {} 時發生異常: {}", fnm, ex.what() );
				  }


			  } );
		  }
		  // 設定錯誤處理
		  tsk.onError( []( const std::exception &ex ) {
			  lg::error( "[vdo] 轉換錯誤: {}", ex.what() );
		  } );
		  tsk.runWaitAll();
		  lg::info( "所有 {} 個檔案處理完成！", tasks.size() );
	  } );

	//------------------------------------------------------------------------
	// main
	//------------------------------------------------------------------------
	ap.arg<string>( "path", "convert path if not set use current", true )
	  .flag( "-c", "clean all .cov files" )
	  .flag( "-cl", "clean all logs" )
	  .flag( "-log", "write log to files" )
	  .flag( "-nvdo", "ignore conv for videos" )
	  .flag( "-nrsz", "don't resize videos" )
	  .flag( "-forceIncr", "force increase pic" )
	  .handle( []( const cli::Args &args ) {
		  auto path = args.get<fs::path>( "path", fs::current_path() );

		  auto depth = rz::io::depths( path );
		  if ( depth <= 3 ) {
			  lg::info( "depth[{}] cannot run... {}", depth, path.string() );
			  return;
		  }

		  bool logFile = false;

		  auto c = args.getOpt<bool>( "-c" );
		  if ( c && *c ) {

			  auto dir = rz::io::IDir::make( path );
			  auto fils = dir->find( ".cov" );
			  for ( auto &fil: fils ) fs::remove( fil );
		  }

		  auto log = args.getOpt<bool>( "-log" );
		  if ( log && *log ) {
			  //auto dir = rz::io::IDir::make( path );
			  //auto logs = dir->find( regex( R"(^_cov_[0-9]+\.log)" ) );
			  //for ( auto &f: logs ) fs::remove( f );
			  logFile = true;
		  }



		  auto nvdo = args.getOpt<bool>( "-nvdo" );
		  if ( nvdo && *nvdo ) cmds::cov::ops.ignoreVideo = true;

		  auto nv = args.getOpt<bool>( "-nv" );
		  if ( nv && *nv ) cmds::cov::ops.ignoreVideo = true;

		  auto resz = args.getOpt<bool>( "-nrsz" );
		  if ( resz && *resz ) cmds::cov::ops.resize = false;

		  auto incr = args.getOpt<bool>( "-forceIncr" );
		  if ( incr && *incr ) cmds::cov::ops.forceIncr = true;


		  try { cmds::cov::startBy( path, logFile ); }
		  catch ( exception &ex ) {
			  lg::error( "[conv] 未捕獲異常, {}", ex.what() );
		  }

	  } );

	auto rst = 0;

	try {
		rst = ap.run( argc, argv );
	}
	catch ( exception &ex ) {
		lg::error( "[conv] 程式外異常, {}", ex.what() );
	}

	if ( rst != 0 ) {
		auto all = cli::err::all();
		for ( auto &e: all ) {
			lg::info( "[ex] {}", e );
		}
	}

	return rst;
}
