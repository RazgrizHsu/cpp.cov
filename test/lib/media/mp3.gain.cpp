#include "doctest.h"

#include <chrono>
#include <string>

#include "../utils.hpp"
#include "rz/idx.h"

#include <iostream>
#include <vector>
#include <filesystem>

#include "mp3g.h"


// 	-g <i>  - apply gain i without doing any analysis
// 	-l 0 <i> - apply gain i to channel 0 (left channel)
// 	          without doing any analysis (ONLY works for STEREO files,
// 	          not Joint Stereo)
// 	-l 1 <i> - apply gain i to channel 1 (right channel)
// 	-e - skip Album analysis, even if multiple files listed
// 	-r - apply Track gain automatically (all files set to equal loudness)
// 	-k - automatically lower Track/Album gain to not clip audio
// 	-a - apply Album gain automatically (files are all from the same
// 	              album: a single gain change is applied to all files, so
// 	              their loudness relative to each other remains unchanged,
// 	              but the average album loudness is normalized)
// 	-i <i> - use Track index i (defaults to 0, for first audio track in file)
// 	-m <i> - modify suggested MP3 gain by integer i
// 	-d <n> - modify suggested dB gain by floating-point n
// 	-c - ignore clipping warning when applying gain
// 	-o - output is a database-friendly tab-delimited list
// 	-t - writes modified data to temp file, then deletes original
// 	     instead of modifying bytes in original file
// 	     A temp file is always used for AAC files.
// 	-q - Quiet mode: no status messages
// 	-p - Preserve original file timestamp
// 	-x - Only find max. amplitude of file
// 	-f - Assume input file is an MPEG 2 Layer III file
// 	     (i.e. don't check for mis-named Layer I or Layer II files)
// 	      This option is ignored for AAC files.
// 	-s c - only check stored tag info (no other processing)
// 	-s d - delete stored tag info (no other processing)
// 	-s s - skip (ignore) stored tag info (do not read or write tags)
// 	-s r - force re-calculation (do not read tag info)
// 	-s i - use ID3v2 tag for MP3 gain info
// 	-s a - use APE tag for MP3 gain info (default)
// 	-u - undo changes made (based on stored tag info)
// 	-w - "wrap" gain change if gain+change > 255 or gain+change < 0
// 	      MP3 only. (use "-? wrap" switch for a complete explanation)
// If you specify -r and -a, only the second one will work
// If you do not specify -c, the program will stop and ask before
//      applying gain change to a file that might clip


TEST_CASE( "mp3gain: call" )
{
    vector<string> paths = {
    	"/Volumes/tmp/test/voice.small.big.mp3",
    	"/Volumes/tmp/test/voice.small.big2.mp3",
    	"/Volumes/tmp/test/voice.small.big3.mp3",
    };

	auto ret = mp3g::mainGain( paths, { .dbGainMod = +5, .applyType = mp3g::ByTrack,
		.onProgress = []( int idx, int percent, unsigned long bytes ) {

			lg::info( "process[{}]: {}% ({})", idx, percent, bytes );
		}
	} );

	lg::info( "[Success]\n{}", ret );
}


TEST_CASE( "mp3gain: anaOnly" )
{
	vector<string> paths = {
		"/Volumes/tmp/test/voice.small.big.mp3",
		"/Volumes/tmp/test/voice.wav",
	};

	auto ret = mp3g::mainGain( paths, { .dbGainMod = +20,
		.onProgress = []( int idx, int percent, unsigned long bytes ) {

			//lg::info( "process[{}]: {}% ({})", idx, percent, bytes );
		}
	} );

	lg::info( "[Success]\n{}", ret );
}



extern "C" {
#include "mp3gain.h"
}

TEST_CASE( "DISABLED_mp3gain: gain" )
{
	string path = "/Volumes/tmp/test/voice.small.big.mp3";
	const char *argv[] = {"pgName", "-r", "-d", "-20", path.c_str(), nullptr};
	int argc = sizeof(argv) / sizeof(argv[0]) - 1;  // 減去 nullptr

	lg::info( "argc[{}], argv: {}", argc, fmt::join(argv, argv+argc,",") );

	//mp3gain_main( argc, const_cast<char**>(argv) );
}



TEST_CASE( "mp3gain: anaDir" )
{
	auto path = "/Volumes/tmp/test/test";
	auto dir = rz::io::IDir::make( path );
	if( !dir ) {
		lg::warn( "not exist: {}", path );
		return;
	}

	auto paths = dir->findF( "*.mp3" );

	auto ret = mp3g::mainGain( paths, { .dbGainMod = +20,
		.onProgress = []( int idx, int percent, unsigned long bytes ) {

			lg::info( "process[{}]: {}% ({})", idx, percent, bytes );
		}
	} );

	if( ret.length() ) {
		lg::info( "analyzed:\n{}", ret );
	}
}
