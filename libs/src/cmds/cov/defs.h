#pragma once

#include <deque>
#include <rz/idx.h>


namespace cmds::cov {

inline const std::string FIL_ConvInfo = ".cov";
inline const std::string FIL_ConvCfgs = ".covcfg";
static const std::filesystem::path PATH_Conf = rz::fmt( "{}/.config/{}", std::getenv( "HOME" ), FIL_ConvCfgs );

class CfgImg : public IJsonDataItem<CfgImg> {
public:
	int q = 60;
	int w = 1920;
	int h = 1680;
	int mxs = 450; //kb
	bool forceScale = false;
	string desc;

	IJsonFields( CfgImg, q, w, h, mxs, forceScale, desc );
};

class CfgVdo : public IJsonDataItem<CfgVdo> {
public:
	int q;
	int mxw;
	string desc;

	CfgVdo( int quality = 66, int mxw = 864, string desc = "" ) 		: q(quality), mxw(mxw), desc(desc) {}

	IJsonFields( CfgVdo, q, mxw, desc );
};


class CovInfo : public IJsonData<CovInfo> {
public:
	long dtm = 0;
	bool vo = 0;
	bool img = 0;
	vector<string> vdos{};

	friend void to_json( Json &j, const CovInfo &json_t ) {
		j["dtm"] = json_t.dtm;
		j["vo"] = json_t.vo;
		j["img"] = json_t.img;
		j["vdos"] = json_t.vdos;
	}
	friend void from_json( const Json &j, CovInfo &json_t ) {
		json_t.dtm = j.value("dtm", json_t.dtm);
		json_t.vo = j.value("vo", json_t.vo);
		json_t.img = j.value("img", json_t.img);
		json_t.vdos = j.value("vdos", json_t.vdos);
	}

	fs::path path;

	static shared_ptr<CovInfo> get( const fs::path &pathDir ) {

		auto path = pathDir;
		if ( !rz::str::contains( path, FIL_ConvInfo ) ) path = path / FIL_ConvInfo;

		shared_ptr<CovInfo> ci = nullptr;

		if ( fs::exists( path ) ) {
			try {
				auto load = CovInfo::load( path );
				if ( load ) ci = std::move( load );
			}
			catch ( exception &ex ) {

			}
		}

		if ( !ci ) {
			ci = make();
			ci->save( pathDir );
		}

		ci->path = path;

		return ci;
	}

	void save( const fs::path &pathDir ) const override {
		auto path = pathDir.string().find( FIL_ConvInfo ) != string::npos ? pathDir : pathDir / FIL_ConvInfo;

		//lg::info( "儲存ConvInfo: {}", path );
		IJsonData::save( path );
	}
	void save() const {

		if ( this->path.empty() ) throw runtime_error( "沒有儲存路徑" );

		//lg::info( "儲存ConvInfo: {}", this->path );
		IJsonData::save( this->path );
	}
};

class CovCfg final : public IJsonData<CovCfg> {
public:
	vector<string> excludes{ };
	map<string, CfgImg> cfgImgs{};
	map<string, CfgVdo> cfgVdos{
		{ "/tmp", CfgVdo( 28, 1680, "") },
	};

	string rootArts = "/tmp";
	vector<string> rmQuoteKeys{ };

	friend void to_json( Json &j, const CovCfg &json_t ) {
		j["excludes"] = json_t.excludes;
		j["cfgImgs"] = json_t.cfgImgs;
		j["cfgVdos"] = json_t.cfgVdos;
		j["rootArts"] = json_t.rootArts;
		j["rmQuoteKeys"] = json_t.rmQuoteKeys;
	}
	friend void from_json( const Json &j, CovCfg &json_t ) {
		json_t.excludes = j.value("excludes", json_t.excludes);
		json_t.cfgImgs = j.value("cfgImgs", json_t.cfgImgs);
		json_t.cfgVdos = j.value("cfgVdos", json_t.cfgVdos);
		json_t.rootArts = j.value("rootArts", json_t.rootArts);
		json_t.rmQuoteKeys = j.value("rmQuoteKeys", json_t.rmQuoteKeys);
	}

	static shared_ptr<CovCfg> load() {

		rz::io::dir::ensure( PATH_Conf );
		shared_ptr<CovCfg> ci = IJsonData::load( PATH_Conf );

		if ( !ci ) {
			ci = make();
			ci->save();
		}

		return ci;
	}

	void save() const {

		//lg::info( "儲存ConvInfo: {}", PATH_Conf );
		IJsonData::save( PATH_Conf );
	}

	CfgImg *findConfigImg( const fs::path &path ) {
		static CfgImg defaultCfg; // 靜態預設設定

		auto p = path;
		CfgImg *ci = nullptr;

		while ( p != "/" && !ci ) {
			auto fnd = cfgImgs.find( p.string() );
			if ( fnd != cfgImgs.end() ) ci = &fnd->second;
			else p = p.parent_path().string();
		}

		if ( !ci ) ci = &defaultCfg;

		return ci;
	}



	CfgImg *findCfgImg(const fs::path &path) {
		string pathStr = path.string();
		string lowerPathStr = pathStr;
		ranges::transform( lowerPathStr, lowerPathStr.begin(), ::tolower);

		for (auto &pair : cfgImgs) {
			string key = pair.first;
			string lowerKey = key;
			ranges::transform( lowerKey, lowerKey.begin(), ::tolower);

			if (lowerPathStr.find(lowerKey) != string::npos) return &pair.second;
		}

		return nullptr;
	}

	CfgVdo *findCfgVdo(const fs::path &path) {
		string pathStr = path.string();
		string lowerPathStr = pathStr;
		ranges::transform( lowerPathStr, lowerPathStr.begin(), ::tolower);

		for (auto &pair : cfgVdos) {
			string key = pair.first;
			string lowerKey = key;
			ranges::transform( lowerKey, lowerKey.begin(), ::tolower);

			if (lowerPathStr.find(lowerKey) != string::npos) {
				auto sec = &pair.second;
				sec->desc += "path( " + lowerKey + " )";
				return sec;
			}
		}

		return nullptr;
	}
};

}
