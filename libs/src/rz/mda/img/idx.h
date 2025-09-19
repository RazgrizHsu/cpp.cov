#pragma once

#include <filesystem>
#include <rz/code/concept.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-anon-enum-enum-conversion"
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#pragma clang diagnostic pop

namespace rz::mda::img {

namespace fs = std::filesystem;

template<typename T>
cv::Mat read( const std::vector<T> &data ) {
	return cv::imdecode( data, cv::IMREAD_UNCHANGED | cv::IMREAD_ANYDEPTH | cv::IMREAD_ANYCOLOR );
}


inline cv::Mat read( std::filesystem::path path ) {
	auto vec = rz::io::fil::read( path );
	cv::Mat img = cv::imdecode( vec, cv::IMREAD_UNCHANGED | cv::IMREAD_ANYDEPTH | cv::IMREAD_ANYCOLOR );
	if ( img.empty() ) throw std::runtime_error( "the img not valid" );
	return img;
}


inline std::string removeNoNeeds( const string &src ) {

	auto str = src;
	vector<string> rgx_switch = {
		R"(^(\([^\)]+\))[ ]?(.+))",
	};
	for ( auto &rgs: rgx_switch ) str = rz::str::rgx::replace( str, rgs, "$2$1" );

	vector<string> rgx_replaces = {
		R"(^ *)",
		R"( *$)",
		R"(\.(zip|rar))",
		R"(^ *\(同人CG集\) *)",
		R"(\(C[0-9]{2,3}\))"
	};

	for ( auto &rgs: rgx_replaces ) str = rz::str::rgx::replace( str, rgs, "" );

	return str;
}

inline void saveJpg( const cv::Mat &img, const fs::path &path, int quality = 68 ) {

	std::vector<int> opts;
	opts.push_back( cv::IMWRITE_JPEG_QUALITY );
	opts.push_back( static_cast<int>(quality) );

	cv::imwrite( path, img, opts );
}

inline void saveWebp( const cv::Mat &img, const fs::path &path, int quality = 68 ) {

	std::vector<int> opts;
	opts.push_back( cv::IMWRITE_WEBP_QUALITY );
	opts.push_back( static_cast<int>(quality) );

	cv::imwrite( path, img, opts );
}

struct reopt {
	bool increase = false;
};

inline cv::Mat resize( const cv::Mat &img, int maxW, int maxH, reopt opt = {} ) {
	auto w = img.cols;
	auto h = img.rows;

	if ( w <= 0 || h <= 0 ) throw runtime_error( rz::fmt( "the image size w[{}] h[{}]", w, h ) );
	//lg::info( "[resize] wh[{}x{}] max[{}x{}]", w, h, maxW, maxH );

	// 檢查是否需要調整大小
	if ( !opt.increase && w <= maxW && h <= maxH ) {
		return img.clone(); // 如果原圖已經足夠小，直接返回副本
	}

	double ws = static_cast<double>(maxW) / w;
	double hs = static_cast<double>(maxH) / h;
	double scale = std::min( ws, hs );

	cv::Size2f originalSize( w, h );
	cv::Size2f targetSize( w * scale, h * scale );

	cv::Mat resized;
	cv::resize( img, resized, targetSize, 0, 0, cv::INTER_AREA );

	return resized;
}


template<typename T>
typename std::enable_if<
	std::is_same<typename std::remove_pointer<typename std::decay<T>::type>::type,
		std::vector<typename std::remove_pointer<typename std::decay<T>::type>::type::value_type>>::value
	||
	std::is_same<typename std::remove_pointer<typename std::decay<T>::type>::type,
		std::deque<typename std::remove_pointer<typename std::decay<T>::type>::type::value_type>>::value,
	cv::Mat
>::type
resize( T &&data, int maxW, int maxH, reopt opt = {} ) {

	const auto &ref = code::getRef( std::forward<T>( data ) );

	auto img = read( ref );
	return resize( img, maxW, maxH, opt );
}



}
