#include "doctest.h"
#include <chrono>
#include <string>

#include "_utils.hpp"

#include "rz/idx.h"




TEST_CASE( "json: base" )
{
	// using json = JSON::Json;
	// json j = { { "name", "tester" }, { "age", 38 } };
	//
	// std::string s = j.dump();
	//
	//
	// CHECK_STREQ( s.c_str(), R"({"age":38,"name":"tester"})" );
}



class Info : public IJsonData<Info> {
public:
	long A = 0;
	bool B = 0;
	bool C = 0;

	IJsonFields( Info, A, B, C )
};


TEST_CASE( "json: custom" )
{
	string strj = R"({"A":8888,"B":false,"C":false})";
	auto v1 = Info::fromJson( strj );

	REQUIRE_EQ( v1.A, 8888 );


	Info cx;
	cx.A = 9999;

	Json js = cx;

	//auto str = js.dump( 2 );
	auto str = cx.toJson().dump();
	REQUIRE_EQ( str, R"({"A":9999,"B":false,"C":false})" );

	js["A"] = 54321;
	auto vv = Info::fromJson( js );

	REQUIRE_EQ( vv.A, 54321 );
}


TEST_CASE( "json: fromFile" ) {

	int count = 0;
RETRY:
	if( ++count >= 3 ) throw std::runtime_error( "failed!" );

	auto path = "/Volumes/tmp/test/test/test.txt";
	auto ci = Info::load( path );
	if( !ci ) {
		lg::info( "該檔案不存在!" );

		Info cn;
		cn.A = 987654321;
		cn.save( path );

		goto RETRY;
	}
	else {

		lg::info( "讀取到的: A:{}", ci->A );

		REQUIRE_EQ( ci->A, 987654321 );
	}
}


#include "cmds/cov/defs.h"

TEST_CASE( "json: CfgImg 兼容性測試" ) {

	// 測試舊格式
	string oldJson = R"({"q":50,"w":1800,"h":1600,"mxs":400,"desc":"test"})";

	SUBCASE( "載入舊設定檔應該成功" ) {
		auto cfg = cmds::cov::CfgImg::fromJson( oldJson );

		REQUIRE_EQ( cfg.q, 50 );
		REQUIRE_EQ( cfg.w, 1800 );
		REQUIRE_EQ( cfg.h, 1600 );
		REQUIRE_EQ( cfg.mxs, 400 );
		REQUIRE_EQ( cfg.forceScale, false );  // 應該使用預設值
		REQUIRE_EQ( cfg.enable, true );       // 新增的 enable 預設值
		REQUIRE_EQ( cfg.desc, "test" );
	}

	// 測試新格式
	string newJson = R"({"q":70,"w":1920,"h":1680,"mxs":450,"forceScale":true,"enable":false,"desc":"new"})";

	SUBCASE( "載入新設定檔應該成功" ) {
		auto cfg = cmds::cov::CfgImg::fromJson( newJson );

		REQUIRE_EQ( cfg.q, 70 );
		REQUIRE_EQ( cfg.w, 1920 );
		REQUIRE_EQ( cfg.h, 1680 );
		REQUIRE_EQ( cfg.mxs, 450 );
		REQUIRE_EQ( cfg.forceScale, true );
		REQUIRE_EQ( cfg.enable, false );
		REQUIRE_EQ( cfg.desc, "new" );
	}
}

TEST_CASE( "json: CfgPath 序列化測試" ) {
	using namespace cmds::cov;

	SUBCASE( "空的 CfgPath" ) {
		CfgPath path;
		path.desc = "test path";

		auto json = path.toJson();
		lg::info("空 path JSON: {}", json.dump());

		REQUIRE_EQ( json["desc"], "test path" );
		REQUIRE( json["img"].is_null() );
		REQUIRE( json["vdo"].is_null() );
	}

	SUBCASE( "有 img 設定的 CfgPath" ) {
		CfgPath path;
		path.desc = "path with img";
		path.img = CfgImg{};
		path.img->enable = false;
		path.img->q = 80;

		auto json = path.toJson();
		lg::info("有 img 但 disable JSON: {}", json.dump());

		REQUIRE_EQ( json["desc"], "path with img" );
		REQUIRE( !json["img"].is_null() );
		REQUIRE_EQ( json["img"]["enable"], false );
		REQUIRE_EQ( json["img"]["q"], 80 );
		REQUIRE( json["vdo"].is_null() );
	}

	SUBCASE( "兩者都有設定的 CfgPath" ) {
		CfgPath path;
		path.desc = "path with both";
		path.img = CfgImg{};
		path.img->enable = true;
		path.vdo = CfgVdo{};
		path.vdo->enable = false;
		path.vdo->q = 40;

		auto json = path.toJson();
		lg::info("兩者都有 JSON: {}", json.dump());

		REQUIRE_EQ( json["desc"], "path with both" );
		REQUIRE( !json["img"].is_null() );
		REQUIRE_EQ( json["img"]["enable"], true );
		REQUIRE( !json["vdo"].is_null() );
		REQUIRE_EQ( json["vdo"]["enable"], false );
		REQUIRE_EQ( json["vdo"]["q"], 40 );
	}

	SUBCASE( "從 JSON 載入測試" ) {
		string testJson = R"({
			"desc": "from json",
			"img": null,
			"vdo": {"q": 50, "mxw": 800, "enable": true, "desc": "test vdo"}
		})";

		auto j = Json::parse(testJson);
		auto path = j.get<CfgPath>();

		lg::info("從 JSON 載入: img={} vdo={}",
			path.img.has_value() ? "有值" : "空",
			path.vdo.has_value() ? "有值" : "空");

		REQUIRE_EQ( path.desc, "from json" );
		REQUIRE( !path.img.has_value() );
		REQUIRE( path.vdo.has_value() );
		REQUIRE_EQ( path.vdo->enable, true );
		REQUIRE_EQ( path.vdo->q, 50 );
		REQUIRE_EQ( path.vdo->mxw, 800 );
		REQUIRE_EQ( path.vdo->desc, "test vdo" );
	}

	SUBCASE( "缺少欄位的 JSON 載入測試" ) {
		string testJson = R"({
			"desc": "missing fields"
		})";

		auto j = Json::parse(testJson);
		auto path = j.get<CfgPath>();

		REQUIRE_EQ( path.desc, "missing fields" );
		REQUIRE( !path.img.has_value() );
		REQUIRE( !path.vdo.has_value() );
	}
}
