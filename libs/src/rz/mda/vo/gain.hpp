#pragma once

#include <mp3g.h>

namespace rz::mda::vo {

using IFnProgReport = std::function<void(int,int,unsigned long)>;

inline void gain( vector<filesystem::path> &paths, IFnProgReport fn = nullptr ) {

	auto onReport = [&]( int idx, int percent, unsigned long bytes ) {

		if( fn ) fn( idx, percent, bytes );
	};
	auto ret = mp3g::mainGain( paths, {
		.dbGainMod = +0,
		.applyType = mp3g::ByTrack,
		.onProgress = onReport
	} );
}

inline void gainAlbum( vector<filesystem::path> &paths, IFnProgReport fn = nullptr ) {

	auto onReport = [&]( int idx, int percent, unsigned long bytes ) {

		if( fn ) fn( idx, percent, bytes );
	};
	auto ret = mp3g::mainGain( paths, {
		.dbGainMod = +0,
		.applyType = mp3g::ByAlbum,
		.onProgress = onReport
	} );
}
inline void gainAlbum( vector<filesystem::path> &paths, double gain, IFnProgReport fn = nullptr ) {

	auto onReport = [&]( int idx, int percent, unsigned long bytes ) {

		if( fn ) fn( idx, percent, bytes );
	};
	auto ret = mp3g::mainGain( paths, {
		.dbGainMod = gain,
		.applyType = mp3g::ByAlbum,
		.onProgress = onReport
	} );

	lg::debug( "gain rst: {}", ret );
}

inline void gain( filesystem::path &path, IFnProgReport fn = nullptr ) {

	vector<filesystem::path> paths;
	paths.push_back( path );

	gain( paths, fn );
}

}