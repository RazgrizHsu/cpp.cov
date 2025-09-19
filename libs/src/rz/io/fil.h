#pragma once

#include <stdexcept>
#include <fstream>
#include <filesystem>
#include <regex>
#include <sstream>
#include <rz/base/number.h>
#include <rz/base/str.h>
#include <spdlog/idx.h>

#include "./dir.h"


namespace rz::io::fil {
namespace fs = std::filesystem;

inline std::string rmext( const std::string &filename ) {
	std::regex extension_regex( "\\.\\w{1,4}$" );
	if ( std::regex_search( filename, extension_regex ) ) return std::regex_replace( filename, extension_regex, "" );
	return filename;
}

inline fs::path stem( const fs::path &filepath ) {
	std::string filename = filepath.filename().string();
	std::string new_filename = rmext( filename );
	return filepath.parent_path() / new_filename;
}

inline std::string nameNoExt( const fs::path &filepath ) {
	std::string filename = filepath.filename().string();
	return rmext( filename );
}

inline std::uintmax_t size( const fs::path &path ) {

	if ( !fs::exists( path ) || !fs::is_regular_file( path ) ) return 0;
	return fs::file_size( path );
}

inline double sizeKB( const fs::path &path ) { return size( path ) / 1024; }

inline double sizeMB( const fs::path &path, int precision = 2 ) {
	auto sz = size( path );
	return rz::toDouble( sz / ( 1024 * 1024 ) );
}


#include <chrono>

#include <chrono>

inline std::shared_ptr<fs::path>
tmp( const std::string &name, bool autoClean = false ) {
	auto dirtmp = rz::io::dir::tmp( true );

	// 獲取當前時間
	auto now = std::chrono::system_clock::now();
	auto now_time_t = std::chrono::system_clock::to_time_t( now );

	// 獲取當天零點時間
	std::tm *now_tm = std::localtime( &now_time_t );
	now_tm->tm_hour = 0;
	now_tm->tm_min = 0;
	now_tm->tm_sec = 0;
	auto today_start = std::chrono::system_clock::from_time_t( std::mktime( now_tm ) );

	// 計算從當天開始到現在的微秒數
	auto duration = now - today_start;
	auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>( duration ).count();

	auto target = dirtmp / rz::fmt( "{}.{}.tmp", name, microseconds );

	if ( fs::exists( target ) ) {
		auto count = 1;
		auto pn = dirtmp / rz::fmt( "{}.{}.{}.tmp", name, microseconds, count++ );
		while ( fs::exists( pn ) ) pn = dirtmp / rz::fmt( "{}.{}.{}.tmp", name, microseconds, count++ );

		std::error_code code;
		fs::rename( target, pn, code );
		if ( code ) throw std::runtime_error( "rename failed, msg[" + code.message() + "], file: " + pn.string() );
	}

	if ( autoClean ) {
		return std::shared_ptr<fs::path>( new fs::path( target ), []( fs::path *p ) {
			std::error_code ec;

			auto path = *p;
			try {
				if ( fs::exists( path ) ) fs::remove( path, ec );
				lg::debug( "已刪除: {}", path.string() );
			}
			catch ( std::exception &ex ) {
				lg::error( "刪除 {} 失敗, {}", path.string(), ex.what() );
			}
			delete p;
		} );
	}

	return std::make_shared<fs::path>( target );
}

// 如果用<string>會逐行讀取, 如果用其他的則是讀取為單個vector<byte>
template<typename T = uint8_t>
std::vector<T> readImpl(const std::string &filePath, bool throwIfNotExist) {
    std::vector<T> vec;

    if constexpr (std::is_same_v<T, std::string>) {
        // 字串類型：按行讀取文本檔案
        std::ifstream file(filePath);

        if (!file.is_open()) {
            if (throwIfNotExist) {
                throw std::runtime_error("無法打開檔案: " + filePath);
            }
            return vec;  // 檔案不存在則返回空向量
        }

        std::string line;
        while (std::getline(file, line)) {
            vec.push_back(line);
        }

        // 檢查是否因讀取錯誤而結束
        if (file.bad()) {
            throw std::runtime_error("讀取檔案時發生錯誤: " + filePath);
        }

        file.close();
    } else {
        // 非字串類型：二進制讀取
        std::ifstream file(filePath, std::ios::binary);

        if (!file.is_open()) {
            if (throwIfNotExist) {
                throw std::runtime_error("無法打開檔案: " + filePath);
            }
            return vec;  // 檔案不存在則返回空向量
        }

        try {
            auto fileSize = std::filesystem::file_size(filePath);

            // 計算元素數量並檢查檔案大小是否合適
            size_t elementCount = fileSize / sizeof(T);
            if (fileSize % sizeof(T) != 0) {
                throw std::runtime_error("檔案大小不是目標類型大小的整數倍: " + filePath);
            }

            // 設定異常狀態
            file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

            // 分配空間並讀取數據
            vec.resize(elementCount);
            file.read(reinterpret_cast<char *>(vec.data()), fileSize);
        }
        catch (const std::filesystem::filesystem_error &e) {
            throw std::runtime_error("獲取檔案大小時發生錯誤: " + filePath + " - " + std::string(e.what()));
        }
        catch (const std::ifstream::failure &e) {
            throw std::runtime_error("讀取檔案時發生錯誤: " + filePath + " - " + std::string(e.what()));
        }
        catch (const std::exception &e) {
            throw std::runtime_error("處理檔案時發生錯誤: " + filePath + " - " + std::string(e.what()));
        }

        file.close();
    }

    return vec;
}

// 拋出異常版本
template<typename T = uint8_t>
std::vector<T> read(const std::string &filePath) {
    return readImpl<T>(filePath, true);
}

// 安全版本，不會在檔案不存在時拋出異常
template<typename T = uint8_t>
std::vector<T> readSafe(const std::string &filePath) {
    return readImpl<T>(filePath, false);
}

template<typename T = std::string>
void save( const std::string &filePath, const T &data ) {

	std::ofstream file( filePath, std::ios::binary );
	if ( !file.is_open() ) throw std::runtime_error( "無法打開檔案進行寫入: " + filePath );

	file.exceptions( std::ofstream::failbit | std::ofstream::badbit );
	try {
		if constexpr ( std::is_same_v<T, std::shared_ptr<std::vector<uint8_t>>> ||
			std::is_same_v<T, std::shared_ptr<std::vector<unsigned char>>> ||
			std::is_same_v<T, std::unique_ptr<std::vector<uint8_t>>> ||
			std::is_same_v<T, std::unique_ptr<std::vector<unsigned char>>> ) {
			if ( data ) file.write( reinterpret_cast<const char *>(data->data()), data->size() );
			else {
				throw std::runtime_error( "智能指針為空" );
			}
		}
		else if constexpr ( std::is_pointer_v<T> ) {
			if constexpr ( std::is_same_v<std::remove_pointer_t<T>, std::vector<uint8_t>> ||
				std::is_same_v<std::remove_pointer_t<T>, std::vector<unsigned char>> ) {
				file.write( reinterpret_cast<const char *>(data->data()), data->size() );
			}
			else throw std::runtime_error( "不支持的指針類型" );

		}
		else if constexpr ( std::is_same_v<T, std::string> ) file.write( data.c_str(), data.size() );
		else if constexpr ( std::is_same_v<T, std::vector<uint8_t>> || std::is_same_v<T, std::vector<unsigned char>> ) {
			file.write( reinterpret_cast<const char *>(data.data()), data.size() );
		}
		else {
			throw std::runtime_error( "不支持的類型" );
			//file.write( reinterpret_cast<const char *>(data.data()), data.size() * sizeof( typename T::value_type ) );
		}
		file.flush();
		file.close();
	}
	catch ( const std::ofstream::failure &e ) {
		file.close();
		throw std::runtime_error( "寫入檔案時發生錯誤: " + std::string( e.what() ) );
	}
}


}
