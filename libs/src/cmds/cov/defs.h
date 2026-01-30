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
	bool enable = true;
	string desc;

	IJsonFields( CfgImg, q, w, h, mxs, forceScale, enable, desc );
};

class CfgVdo : public IJsonDataItem<CfgVdo> {
public:
	int q = 66;
	int mxw = 864;
	bool enable = true;
	string desc;

	CfgVdo( int quality = 66, int mxw = 864, string desc = "" ) 		: q(quality), mxw(mxw), desc(desc) {}

	IJsonFields( CfgVdo, q, mxw, enable, desc );
};

class CfgPath : public IJsonDataItem<CfgPath> {
public:
	string desc;
	optional<CfgImg> img;
	optional<CfgVdo> vdo;

	friend void to_json( Json &j, const CfgPath &json_t ) {
		j["desc"] = json_t.desc;
		if ( json_t.img.has_value() ) {
			j["img"] = json_t.img.value();
		} else {
			j["img"] = nullptr;
		}
		if ( json_t.vdo.has_value() ) {
			j["vdo"] = json_t.vdo.value();
		} else {
			j["vdo"] = nullptr;
		}
	}
	friend void from_json( const Json &j, CfgPath &json_t ) {
		json_t.desc = j.value("desc", json_t.desc);
		if ( j.contains("img") && !j["img"].is_null() ) {
			json_t.img = j["img"].get<CfgImg>();
		} else {
			json_t.img = std::nullopt;
		}
		if ( j.contains("vdo") && !j["vdo"].is_null() ) {
			json_t.vdo = j["vdo"].get<CfgVdo>();
		} else {
			json_t.vdo = std::nullopt;
		}
	}
};

class VdoInfo : public IJsonDataItem<VdoInfo> {
public:
	string name;
	double szo = 0.0;  // size old
	double szn = 0.0;  // size new
	int q = 0;         // quality

	IJsonFields( VdoInfo, name, szo, szn, q );
};

class CovInfo : public IJsonData<CovInfo> {
public:
	long dtm = 0;
	bool vo = 0;
	bool img = 0;
	vector<string> vdos{};
	vector<VdoInfo> vdoInfos{};

	friend void to_json( Json &j, const CovInfo &json_t ) {
		j["dtm"] = json_t.dtm;
		j["vo"] = json_t.vo;
		j["img"] = json_t.img;
		j["vdos"] = json_t.vdos;
		j["vdoInfos"] = json_t.vdoInfos;
	}
	friend void from_json( const Json &j, CovInfo &json_t ) {
		json_t.dtm = j.value("dtm", json_t.dtm);
		json_t.vo = j.value("vo", json_t.vo);
		json_t.img = j.value("img", json_t.img);
		json_t.vdos = j.value("vdos", json_t.vdos);
		json_t.vdoInfos = j.value("vdoInfos", json_t.vdoInfos);
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
	map<string, CfgPath> paths{};

	string rootArts = "/tmp";
	vector<string> rmQuoteKeys{ };

	friend void to_json( Json &j, const CovCfg &json_t ) {
		j["excludes"] = json_t.excludes;
		j["paths"] = json_t.paths;
		j["rootArts"] = json_t.rootArts;
		j["rmQuoteKeys"] = json_t.rmQuoteKeys;
	}
	friend void from_json( const Json &j, CovCfg &json_t ) {
		json_t.excludes = j.value("excludes", json_t.excludes);
		json_t.paths = j.value("paths", json_t.paths);
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
		static CfgImg defaultCfg;

		auto p = path;

		while ( p != "/" ) {
			auto fnd = paths.find( p.string() );
			if ( fnd != paths.end() && fnd->second.img.has_value() ) {
				return &fnd->second.img.value();
			}
			p = p.parent_path().string();
		}

		return &defaultCfg;
	}

	CfgImg *findCfgImg(const fs::path &path) {
		string pathStr = path.string();
		string lowerPathStr = pathStr;
		ranges::transform( lowerPathStr, lowerPathStr.begin(), ::tolower);

		for (auto &pair : paths) {
			if (!pair.second.img.has_value()) continue;

			string key = pair.first;
			string lowerKey = key;
			ranges::transform( lowerKey, lowerKey.begin(), ::tolower);

			if (lowerPathStr.find(lowerKey) != string::npos) {
				return &pair.second.img.value();
			}
		}

		return nullptr;
	}

	CfgVdo *findCfgVdo(const fs::path &path) {
		string pathStr = path.string();
		string lowerPathStr = pathStr;
		ranges::transform( lowerPathStr, lowerPathStr.begin(), ::tolower);

		for (auto &pair : paths) {
			if (!pair.second.vdo.has_value()) continue;

			string key = pair.first;
			string lowerKey = key;
			ranges::transform( lowerKey, lowerKey.begin(), ::tolower);

			if (lowerPathStr.find(lowerKey) != string::npos) {
				auto sec = &pair.second.vdo.value();
				sec->desc += "path( " + lowerKey + " )";
				return sec;
			}
		}

		return nullptr;
	}

};

}
