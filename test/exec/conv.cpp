#include "doctest.h"

#include <algorithm>
#include <deque>

#include <rz/idx.h>
#include <cmds/cov/idx.h>

using namespace rz;

TEST_CASE( "test: basic" ) {

	auto ci = cmds::cov::CovCfg::load();
	if( !ci ) { lg::error( "load failed?" ); return; }


	lg::info( "excludes.size: {}", ci->excludes.size() );

	// for( auto &pair : ci->dics ) {
	// 	lg::info( "key[{}] v[{}]", pair.first, pair.second.x );
	// }

	auto cii = ci->findConfigImg( "/Volumes/tmp/test" );

	auto ph = "/Volumes/tmp/test/test";

	auto cf = ci->findConfigImg( ph );

	lg::info( "fnd: {}", cf->toJson().dump() );


	ci->save();

}

TEST_CASE( "config: load" ) {

	string jsonWithExtraField = R"({
		"excludes": [],
		"cfgImgs": {},
		"cfgVdos": {
			"/tmp": {"q": 20, "mxw": 9999, "desc": ""}
		},
		"rootArts": "/tmpzzzz",
		"rmQuoteKeys": ["somekey", "anotherkey"]
	})";

	try {
		Json j = Json::parse(jsonWithExtraField);

		// 現在使用修改後的
		auto cfgFromJson = j.get<cmds::cov::CovCfg>();
		auto cfg = std::make_shared<cmds::cov::CovCfg>(cfgFromJson);

		REQUIRE(cfg != nullptr);

		lg::info("容錯解析的設定: {}", cfg->toJson().dump(2));

		auto savedJson = cfg->toJson();
		REQUIRE(savedJson.contains("rootArts"));
		REQUIRE(savedJson.contains("excludes"));
		REQUIRE(savedJson.contains("cfgImgs"));
		REQUIRE(savedJson.contains("cfgVdos"));
		REQUIRE(!savedJson.contains("rmQuoteKeys"));

		lg::info("向後相容性測試通過：未知欄位不影響正常讀取且不會被保存");

	} catch (const std::exception& e) {
		lg::error("向後相容性測試失敗: {}", e.what());
		REQUIRE(false);
	}
}

TEST_CASE( "cov: sub_gain" )
{
	auto mp3s = rz::io::dir::files( "/Volumes/tmp/test/test" );

	auto cMax = 100 * mp3s.size();
	lg::info( "[arc:vo] size[{}] cmax[{}] gain: {}", mp3s.size(), cMax, str::join( mp3s ) );
	auto bar = rz::code::prog::makeBar( rz::fmt( " gain:{}", mp3s.size() ), {cMax} );
	mda::vo::gainAlbum( mp3s, -5, [&]( int idx, int percent, unsigned long ) {

		auto now = idx <= 0 ? percent : ( percent + ( idx * 100 ) );

		bar->setProgress( now );
		bar->setPrefix( rz::fmt( " gain:{}/{}", idx, mp3s.size() ) );

	} );
}
