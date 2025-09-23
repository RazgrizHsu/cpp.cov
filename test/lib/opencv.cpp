#include "doctest.h"

#include <chrono>
#include <string>

#include "utils.hpp"
#include "opencv2/opencv.hpp"
#include "rz/idx.h"


TEST_CASE( "opencv: base" )
{
	cv::Mat img = cv::imread("002.webp");


	if( img.empty() ){ lg::warn( "not found" ); return; }

	double quality = 40.0; // 設定為60%
	std::vector<int> compression_params;
	compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
	compression_params.push_back(static_cast<int>(quality));

	// 儲存為JPG
	bool success = cv::imwrite("output_image.jpg", img, compression_params);

	if (success) {
		std::cout << "圖片已成功儲存為JPG，畫質為" << quality << "%" << std::endl;
	} else {
		std::cout << "儲存圖片失敗" << std::endl;
	}
}


TEST_CASE( "opencv: mem" ) {

	// 假設您已經有一個加載到記憶體中的圖像 Mat image

	// 假設您已經有了包含圖像數據的 vector
	//std::vector<unsigned char> buffer;  // 這裡應該包含您的圖像數據

	auto img = rz::mda::img::read( "/Volumes/tmp/test/test.jpg" );



	// 定義壓縮質量（0-100，100為最高質量）
	int jpegQuality = 68; // 這裡設定為75%的質量


	std::vector<int> compression_params;
	compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
	compression_params.push_back(static_cast<int>(jpegQuality));


	cv::imwrite( "/Volumes/tmp/test/test-done.jpg", img, compression_params );
}

TEST_CASE( "opencv: resize" )
{
	auto img = rz::mda::img::read( "/Volumes/tmp/test/test.jpg" );
	auto w = img.cols;
	auto h = img.rows;

	cv::Mat resized;
	int maxW = 1680;
	int maxH = 1280;


	double ws = static_cast<double>(maxW) / w;
	double hs = static_cast<double>(maxH) / h;
	double scale = std::min(ws, hs);

	int newW = static_cast<int>(w * scale);
	int newH = static_cast<int>(h * scale);

	cv::resize( img, resized, cv::Size(newW, newH), 0, 0, cv::INTER_AREA);


	// 定義壓縮質量（0-100，100為最高質量）
	int jpegQuality = 68; // 這裡設定為75%的質量


	std::vector<int> opts;
	opts.push_back(cv::IMWRITE_JPEG_QUALITY);
	opts.push_back(static_cast<int>(jpegQuality));


	cv::imwrite( "/Volumes/tmp/test/test-done.jpg", resized, opts );
}

TEST_CASE( "opencv: resize_mem" ) {

	using namespace rz::mda;
	auto vec = rz::io::fil::read( "/Volumes/tmp/test/test.jpg" );
	auto res = img::read( vec );

	auto img = img::resize( res, 500, 300 );

	img::saveJpg( img, "/Volumes/tmp/test/test-ok.jpg", 68 );
}

TEST_CASE( "opencv: solid_color_detection" ) {
	using namespace rz::mda;

	cv::Mat whiteImg( 100, 100, CV_8UC3, cv::Scalar( 255, 255, 255 ) );
	cv::Mat blackImg( 100, 100, CV_8UC3, cv::Scalar( 0, 0, 0 ) );
	cv::Mat grayImg( 100, 100, CV_8UC3, cv::Scalar( 128, 128, 128 ) );
	cv::Mat redImg( 100, 100, CV_8UC3, cv::Scalar( 0, 0, 255 ) );

	cv::Mat noiseImg( 100, 100, CV_8UC3, cv::Scalar( 128, 128, 128 ) );
	cv::randu( noiseImg, cv::Scalar( 100, 100, 100 ), cv::Scalar( 156, 156, 156 ) );

	bool whiteResult = img::isSolidColorImage( whiteImg );
	bool blackResult = img::isSolidColorImage( blackImg );
	bool grayResult = img::isSolidColorImage( grayImg );
	bool redResult = img::isSolidColorImage( redImg );
	bool noiseResult = img::isSolidColorImage( noiseImg );

	lg::info( "[test] 白色單色檢測: {}", whiteResult );
	lg::info( "[test] 黑色單色檢測: {}", blackResult );
	lg::info( "[test] 灰色單色檢測: {}", grayResult );
	lg::info( "[test] 紅色單色檢測: {}", redResult );
	lg::info( "[test] 雜訊圖檢測: {}", noiseResult );

	CHECK( whiteResult == true );
	CHECK( blackResult == true );
	CHECK( grayResult == true );
	CHECK( redResult == true );
	CHECK( noiseResult == false );

	lg::info( "[test] 單色圖片檢測測試完成" );
}

