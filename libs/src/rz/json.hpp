#pragma once

#include <string>
#include <ostream>
#include <spdlog/idx.h>
#include "./@lib/json.hpp"

namespace fs = std::filesystem;
using Json = JSON::Json;

template<typename Derived>
class IJsonDataItem {
public:
	Json toJson() { return *static_cast<const Derived *>(this); }

	static Derived fromJson( Json json ) { return json.get<Derived>(); }
	static Derived fromJson( std::string jsonStr ) { return Json::parse( jsonStr ).get<Derived>(); }

	static std::unique_ptr<Derived> make() { return make_unique<Derived>(); }
};

template<typename Derived>
class IJsonData : public IJsonDataItem<Derived> {
public:

	virtual void save( const fs::path &path ) const {

		Json j;
		try {
			j = *static_cast<const Derived *>(this);
		}
		catch( std::exception &ex ) {
			auto msg = rz::fmt( "轉換成json失敗, {}", ex.what() );
			throw runtime_error( msg );
		}

		try {
			std::ofstream file( path );
			if ( file.is_open() ) {
				file << j.dump( 4 );
				file.close();
				return;
			}
			throw std::runtime_error( "無法開啟檔案" );
		}
		catch ( std::exception &ex ) {
			auto msg = rz::fmt( "寫入檔案[{}]失敗, {}", path.string(), ex.what() );
			throw std::runtime_error( msg );
		}
	}



	static std::shared_ptr<Derived> load( const fs::path &path ) {
		std::ifstream file( path );
		if ( file.is_open() ) {
			try {
				Json j;
				file >> j;
				file.close();

				auto instance = j.get<Derived>();
				return std::make_shared<Derived>( instance );
			}
			catch( std::exception &ex ) {
				lg::warn( "[JSON:{}] load failed, {}", __PRETTY_FUNCTION__, ex.what() );
			}
		}

		return nullptr;
	}

	virtual ~IJsonData() = default;
};


#define IJsonFields(...) \
	public: \
	DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT( __VA_ARGS__ )
	//DEFINE_TYPE_INTRUSIVE( __VA_ARGS__ ) //嚴格模式
