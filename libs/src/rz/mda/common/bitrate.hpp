#pragma once

#include <rz/code/concept.h>
#include <rz/mda/info.hpp>
#include <string>
#include <map>

namespace rz::mda {

namespace fs = std::filesystem;

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
inline string getInfo( const fs::path &path ) {
	auto cmd = rz::fmt( R"(ffmpeg -i {} 2>&1)", rz::escapeShell( path ) );
	auto *pipe = popen( cmd.c_str(), "r" );
	if ( !pipe ) return 0;

	char buffer[128];
	string rst = "";
	while ( fgets( buffer, 128, pipe ) != nullptr ) rst += buffer;

	pclose( pipe );

	return rst;
}

inline double getBitRate( const fs::path &path ) {
	string info = getInfo( path );
	std::regex re( "bitrate: ([0-9]+)" );
	std::smatch match;
	auto str = std::regex_search( info, match, re ) ? match[1].str() : "0";
	return std::stod( str );
}

inline double getDurationSecs( const fs::path &path ) {
	auto cmd = rz::fmt( R"(ffmpeg -i {} 2>&1 | grep 'Duration:')", rz::escapeShell( path ) );
	auto *pipe = popen( cmd.c_str(), "r" );
	if ( !pipe ) return 0;

	char buffer[128];
	string rst = "";
	while ( fgets( buffer, 128, pipe ) != nullptr ) rst += buffer;

	pclose( pipe );

	std::regex re( R"(Duration: ([0-9.:]+))" );
	std::smatch match;
	auto str = std::regex_search( rst, match, re ) ? match[1].str() : "00:00:00";

	auto idu = rz::dt::IDU( str );
	return idu.secs();
}



// Extract thumbnail from video
inline bool extractThumbnail( const fs::path &vidPath, const fs::path &outPath, double time = -1 ) {
	string timeArg = ( time >= 0 ) ? rz::fmt( "-ss {}", time ) : "-ss 00:00:05";

	auto cmd = rz::fmt( "ffmpeg {} -i {} -vframes 1 -q:v 2 {} -y 2>&1",
		timeArg, rz::escapeShell( vidPath ), rz::escapeShell( outPath ) );

	auto *pipe = popen( cmd.c_str(), "r" );
	if ( !pipe ) return false;
	pclose( pipe );

	return fs::exists( outPath );
}

// Extract audio from video
inline bool extractAudio( const fs::path &vidPath, const fs::path &outPath, const string &fmt = "mp3" ) {
	auto cmd = rz::fmt( "ffmpeg -i {} -vn -acodec {} {} -y 2>&1",
		rz::escapeShell( vidPath ),
		fmt == "mp3" ? "libmp3lame" : "copy",
		rz::escapeShell( outPath ) );

	auto *pipe = popen( cmd.c_str(), "r" );
	if ( !pipe ) return false;
	pclose( pipe );

	return fs::exists( outPath );
}

// Check if video is playable
inline bool isVideoPlayable( const fs::path &path ) {
	auto cmd = rz::fmt( "ffmpeg -v error -i {} -f null - 2>&1", rz::escapeShell( path ) );

	auto *pipe = popen( cmd.c_str(), "r" );
	if ( !pipe ) return false;

	char buf[128];
	string result;
	while ( fgets( buf, 128, pipe ) != nullptr ) result += buf;
	pclose( pipe );

	return result.empty();
}


// 舊版
// struct VideoInfo {
// 	string type = "";
// 	int w = 0;
// 	int h = 0;
// };
//
//
// // Stream #0:0[0x1](eng): Video: h264 (Main) (avc1 / 0x31637661), yuv420p(progressive), 1280x720 [SAR 1:1 DAR 16:9], 14662 kb/s, 29.97 fps, 29.97 tbr, 30k tbn (default)
// inline VideoInfo getVideoInfo( const fs::path &path ) {
// 	string info = getInfo( path );
//
// 	VideoInfo vi;
//
// 	std::regex re( "Video:\\s*(\\w+)" );
// 	std::smatch match;
// 	if ( std::regex_search( info, match, re ) && match.size() > 1 ) vi.type = match[1].str();
//
// 	re = std::regex( "Video:.*?\\b(\\d{3,4})x(\\d{3,4})\\b" );
// 	if ( std::regex_search( info, match, re ) && match.size() > 2 ) {
// 		vi.w = std::stoi( match[1].str() );
// 		vi.h = std::stoi( match[2].str() );
// 	}
//
// 	return vi;
// }

}






#include <rz/mda/info.hpp>
#include <string>
#include <vector>
#include <optional>


//------------------------------------------------------------------------
// VdoInfo
//------------------------------------------------------------------------
namespace rz::mda {
namespace fs = std::filesystem;
using std::string;

inline string rx(const string &s, const string &pat, int g = 1) {
    std::smatch m;
    if (std::regex_search(s, m, std::regex(pat)) && m.size() > g) return m[g].str();
    return "";
}

struct StreamInfo {
    int idx = 0;
    string type = "";     // "Video", "Audio", "Subtitle"
    string codec = "";
    string lang = "";
    std::map<string, string> props;
};

struct VideoInfo {
    string codec = "";     // Video codec type (h264, h265, etc.)
    int w = 0;            // Width in pixels
    int h = 0;            // Height in pixels
    double fps = 0.0;     // Frames per second
    double frameRate = 0.0; // Frames per second (alternative representation)
    double duration = 0.0;  // Duration in seconds
    double bitRate = 0.0;   // Bitrate in kb/s
    bool isHDR = false;   // Whether the video has HDR
    string pixFmt = "";   // Pixel format (yuv420p, etc.)
    string aspectRatio = ""; // Display aspect ratio (e.g., "16:9")
    std::vector<StreamInfo> streams; // All streams in the video file

	string raw = "";
};

inline void processProps(StreamInfo &si, const string &line, bool log = false) {
    auto &p = si.props;

    if (si.type == "Video") {
        p["w"] = rx(line, "(\\d+)x(\\d+)", 1);
        p["h"] = rx(line, "(\\d+)x(\\d+)", 2);
        p["fps"] = rx(line, R"((\d+(?:\.\d+)?)\s*(?:fps|tbr))");
        p["pixfmt"] = rx(line, R"((yuv\w+(?:p\d+le)?|rgb\w+|bgr\w+))");
        p["hdr"] = (line.find("HDR") != string::npos || line.find("hdr10") != string::npos ||
                   line.find("hlg") != string::npos || line.find("10le") != string::npos) ? "1" : "0";
        p["ar"] = rx(line, R"(DAR (\d+:\d+))");
        if (p["ar"].empty()) p["ar"] = rx(line, R"(\[SAR \d+:\d+ DAR (\d+:\d+)\])");
        p["br"] = rx(line, "(\\d+)\\s*kb/s");

        if (log) {
            if (!p["w"].empty()) lg::info("解析: 分辨率={}x{}", p["w"], p["h"]);
            if (!p["fps"].empty()) lg::info("解析: 幀率={}", p["fps"]);
            if (!p["pixfmt"].empty()) lg::info("解析: 像素格式={}", p["pixfmt"]);
            if (p["hdr"] == "1") lg::info("解析: HDR=是");
            if (!p["ar"].empty()) lg::info("解析: 寬高比={}", p["ar"]);
            if (!p["br"].empty()) lg::info("解析: 比特率={}", p["br"]);
        }
    }
    else if (si.type == "Audio") {
        p["rate"] = rx(line, "(\\d+)\\s*Hz");
        p["br"] = rx(line, "(\\d+)\\s*kb/s");

        if (line.find("stereo") != string::npos) p["ch"] = "2";
        else if (line.find("mono") != string::npos) p["ch"] = "1";
        else if (line.find("5.1") != string::npos) p["ch"] = "6";
        else if (line.find("7.1") != string::npos) p["ch"] = "8";
        else p["ch"] = rx(line, R"((\d+)(?:\.\d+)?\s*ch)");

        if (log) {
            if (!p["rate"].empty()) lg::info("解析: 採樣率={}", p["rate"]);
            if (!p["br"].empty()) lg::info("解析: 比特率={}", p["br"]);
            if (!p["ch"].empty()) lg::info("解析: 聲道數={}", p["ch"]);
        }
    }
}

inline std::vector<StreamInfo> parseStreams(const string &info, bool log = false) {
	std::vector<StreamInfo> streams;

	// 使用更寬鬆的正則表達式，不依賴行尾匹配
	std::regex lineRx(R"(Stream #\d+:\d+.*)");
	std::vector<string> streamLines;

	// 使用sregex_iterator查找所有匹配項
	for (std::sregex_iterator it(info.begin(), info.end(), lineRx), end; it != end; ++it) {
		streamLines.push_back(it->str());
	}

	if (log) lg::info("找到 {} 個流信息行", streamLines.size());

	if ( streamLines.empty() ) lg::warn( "從資訊中找不到流訊息:\n{}", info );

	for (const auto& line : streamLines) {
		StreamInfo si;
		std::smatch match;

		if (std::regex_search(line, match, std::regex(R"(Stream #(\d+):(\d+))")))
			si.idx = std::stoi(match[2]);

		if (std::regex_search(line, match, std::regex(R"(\((\w+)\))")))
			si.lang = match[1];

		if (std::regex_search(line, match, std::regex(R"(: (\w+): (.+?)(?:,|\(|$))"))) {
			si.type = match[1];
			string fullCodec = match[2];

			if (std::regex_search(fullCodec, match, std::regex(R"((\w+)(?:\s*\(([^)]+)\))?)"))) {
				si.codec = match[1];
				if (match.size() > 2 && match[2].matched)
					si.props["profile"] = match[2];
			} else si.codec = fullCodec;
		}

		processProps(si, line, log);

		if (log) {
			lg::info("流: 索引={}, 類型={}, 編解碼器={}, 語言={}", si.idx, si.type, si.codec, si.lang);
			for (const auto& [k, v] : si.props) lg::info("  屬性: {}={}", k, v);
		}

		streams.push_back(si);
	}

	return streams;
}

inline VideoInfo getVideoInfo(const fs::path &path, bool log = false) {
    string info = getInfo(path);
    VideoInfo vi;

	vi.raw = info;

    if (log) lg::info("獲取到視頻信息:\n{}", info);

    vi.streams = parseStreams(info, log);
    if (log) lg::info("解析到 {} 個流", vi.streams.size());

    // 編解碼器
    std::regex reCodec(R"(Video:\s*([^,()\s]+))");
    std::smatch match;
    if (std::regex_search(info, match, reCodec)) vi.codec = match[1];

    // 分辨率
    if (std::regex_search(info, match, std::regex(R"(Video:.*?\b(\d{3,4})x(\d{3,4})\b)"))) {
        vi.w = std::stoi(match[1]);
        vi.h = std::stoi(match[2]);
    }

    // 幀率
    if (std::regex_search(info, match, std::regex(R"((\d+(?:\.\d+)?)\s*(?:fps|tbr))")))
        vi.fps = vi.frameRate = std::stod(match[1]);

    // 持續時間
    if (std::regex_search(info, match, std::regex(R"(Duration:\s*(\d+):(\d+):(\d+(?:\.\d+)?))")))
        vi.duration = std::stoi(match[1]) * 3600 + std::stoi(match[2]) * 60 + std::stod(match[3]);

    // 比特率
    if (std::regex_search(info, match, std::regex(R"(bitrate:\s*(\d+(?:\.\d+)?)\s*kb/s)")))
        vi.bitRate = std::stod(match[1]);

    // 像素格式
    if (std::regex_search(info, match, std::regex(R"(Video:.*?(yuv\w+(?:p\d+le)?|rgb\w+|bgr\w+))")))
        vi.pixFmt = match[1];

    // HDR
    vi.isHDR = info.find("HDR") != string::npos || info.find("hdr10") != string::npos ||
              info.find("hlg") != string::npos || info.find("10le") != string::npos ||
              vi.pixFmt.find("p10le") != string::npos;

    // 寬高比
    if (std::regex_search(info, match, std::regex(R"(DAR (\d+:\d+))")))
        vi.aspectRatio = match[1];
    else if (std::regex_search(info, match, std::regex(R"(\[SAR \d+:\d+ DAR (\d+:\d+)\])")))
        vi.aspectRatio = match[1];

    // 從流信息中提取缺失的數據
    for (const auto &stream : vi.streams) {
        if (stream.type == "Video" && vi.codec.empty()) {
            vi.codec = stream.codec;

            auto &p = stream.props;
            if (!p.at("w").empty() && (vi.w == 0 || vi.h == 0)) {
                vi.w = std::stoi(p.at("w"));
                vi.h = std::stoi(p.at("h"));
            }

            if (!p.at("fps").empty() && vi.fps == 0.0)
                vi.fps = vi.frameRate = std::stod(p.at("fps"));

            if (!p.at("pixfmt").empty() && vi.pixFmt.empty())
                vi.pixFmt = p.at("pixfmt");

            if (p.at("hdr") == "1")
                vi.isHDR = true;

            if (!p.at("ar").empty() && vi.aspectRatio.empty())
                vi.aspectRatio = p.at("ar");

            if (!p.at("br").empty() && vi.bitRate == 0.0)
                vi.bitRate = std::stod(p.at("br"));
        }
    }

    return vi;
}

}