#include "doctest.h"
#include <rz/idx.h>

TEST_CASE( "cmd: test" ) {
	auto actOut = []( bool isStdOut, const std::string &line ) {

		lg::info( "[{}] {}", isStdOut ? "STD" : "ERR", line );
	};


	// 執行 ls 命令
	int result = rz::exec( "ls -la", actOut );
	std::cout << "ls command returned: " << result << std::endl;

	result = rz::exec( "sleep 1" );

	// 執行可能產生錯誤的命令
	result = rz::exec( "ls nonexistent_file", actOut );
	std::cout << "Command with error returned: " << result << std::endl;
}



TEST_CASE( "cmd: parse" ) {
	string line = " size=    1024KiB time=00:00:53.08 bitrate= 158.0kbits/s speed= 105x";

	regex rxDur( R"(time=([^ ]+))" );
	smatch mth;
	if( regex_search( line, mth, rxDur ) ) {
		if( mth[1].matched ) {
			string v = mth[1];
			lg::info( "match time: [{}]", v );
		}
	}
}




TEST_CASE( "dir: use_tmp" ) {

	using namespace rz::io;

	auto pathTmp = fil::tmp( "test" );
	fil::save( *pathTmp, "" );

	lg::info( "get tmp path: {}", (*pathTmp).string() );

}


// 通用的基準測試函式
template<typename Func>
double benchmark(Func&& func, int iterations = 1) {
	std::vector<double> durations;
	durations.reserve(iterations);

	for (int i = 0; i < iterations; ++i) {
		auto start = std::chrono::high_resolution_clock::now();

		std::thread t(func);
		t.join();

		auto end = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double, std::milli> duration = end - start;
		durations.push_back(duration.count());
	}

	// 計算平均執行時間
	double average = std::accumulate(durations.begin(), durations.end(), 0.0) / iterations;

	return average;
}

TEST_CASE( "cmd: bench" ) {

	auto count = 10;

	auto path = "/Volumes/tmp/test/0.wav";


	auto result2 = benchmark([&]() {
		auto info = rz::mda::getDurationSecs( path );
	}, count);

	std::cout << "Average time for example 2: " << result2 << " ms" << std::endl;


	// auto result3 = benchmark([&]() {
	// 	auto info = rz::mda::vdo::getFFPInfo( path );
	// 	string str = "";
	// 	if( info->format.duration ) str = *info->format.duration;
	// }, count);
	// std::cout << "Average time for example 3: " << result3 << " ms" << std::endl;
}


TEST_CASE( "cmd: voice" ) {

	auto path = "/Volumes/tmp/test/0.wav";


	//測試2
	// auto info = rz::mda::vdo::getFFPInfo( path );
	// string str = "";
	// if( info->format.duration ) str = *info->format.duration;
	// //lg::info( "dur[{}] by info", str );


}
