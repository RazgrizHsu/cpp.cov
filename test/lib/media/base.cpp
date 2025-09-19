#include "doctest.h"
#include "rz/idx.h"

#include <vector>
#include <stdexcept>

using namespace rz::mda;

TEST_CASE( "media_base: getInfos" )
{
	fs::path p = "/Volumes/tmp/test/voice.wav";

	lg::info( "bitrate(str): {}", rz::mda::getBitRate( p ) );

	p = "/Volumes/tmp/test/test/01.mp3";

	lg::info( "duration(str): {}", rz::mda::getDurationSecs( p ) );
}

TEST_CASE( "media: test" )
{
	auto paths = {
		"/Volumes/tmp/test.mp4",
	};

	for (const auto& path : paths) {
		auto vi = rz::mda::getVideoInfo(path, false);
		lg::info("{}|{}|{}| w[{}] from {}", vi.codec, vi.bitRate, vi.pixFmt, vi.w, path);
	}
}
