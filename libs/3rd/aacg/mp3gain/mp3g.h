#pragma once

namespace mp3g {


using IFnOnProgress = std::function<void(int,int,unsigned long)>;


typedef enum
{
	storeTime,
	setStoredTime
}
timeAction;

typedef enum { NoId=0, LEFT=1, RIGHT=2 } ChannelType;
// -l <n> <i> - apply gain i to channel (<n> 0:left,1:right) without doing any analysis (ONLY works for STEREO files, not Joint Stereo)

typedef enum { NoType = 0, ByTrack = 1, ByAlbum = 2 } ByType;



struct sOpt {
	bool checkTagOnly = 0;
	bool deleteTag = 0;
	bool skipTag = 0;
	bool forceRecalculateTag = 0;
	bool useId3 = 0;
};

struct gainOpts {
	bool album = 0;
	double dbGainMod = 0.0;	// 	-d <n> - modify suggested dB gain by floating-point n
	int mp3GainMod = 0; //-m <i> - modify suggested MP3 gain by integer i
	bool keepOrigDT = 1; //-p - Preserve original file timestamp

	bool ignoreClipWarn = 1;	// -c - If you do not specify -c, the program will stop and ask before
	//      applying gain change to a file that might clip

	bool reckless = 0;		// 	-f - Assume input file is an MPEG 2 Layer III file (i.e. don't check for mis-named Layer I or Layer II files) This option is ignored for AAC files.
	bool quiteMode = 0;		//quite mode (no-output)

	bool autoClip = 0; // 	-k - automatically lower Track/Album gain to not clip audio

	bool dbFormat = 0; // 	-o - output is a database-friendly tab-delimited list

	int trackIndex = 0; // 	-i <i> - use Track index i (defaults to 0, for first audio track in file)

	bool undoChanges = 0; // 	-u - undo changes made (based on stored tag info)

	bool useTmp = 0; // 	-t - use tmp file (AAC files always)

	bool warpGain = 0; // 	-w - "wrap" gain change if gain+change > 255 or gain+change < 0 ((MP3 only

	bool onlyMaxAmp = 0; // 	-x - Only find max. amplitude of file
	bool anaTrack = 0; // 	-e - skip Album analysis, even if multiple files listed


	// -l 跟 -g 不能同時用, 如果有指定type就要指定gain

	ChannelType directChId = NoId;	//-l <L or R>
	int directGain = 0;				// 	-g <i>  - apply gain i without doing any analysis

	// If you specify -r and -a, only the second one will work
	ByType applyType = NoType; // ByTrack(-r) or ByAlbum(-a)
	// 	-r - apply Track gain automatically (all files set to equal loudness)
	// 	-a - apply Album gain automatically (files are all from the same
	// 	              album: a single gain change is applied to all files, so
	// 	              their loudness relative to each other remains unchanged,
	// 	              but the average album loudness is normalized)

	const sOpt *s;

	const IFnOnProgress onProgress;
};



// 	-s c - only check stored tag info (no other processing)
// 	-s d - delete stored tag info (no other processing)
// 	-s s - skip (ignore) stored tag info (do not read or write tags)
// 	-s r - force re-calculation (do not read tag info)
// 	-s i - use ID3v2 tag for MP3 gain info
// 	-s a - use APE tag for MP3 gain info (default)


std::string mainGain( std::vector<std::string> &paths, const gainOpts& inOpts );
std::string mainGain( std::vector<std::filesystem::path> &paths, const gainOpts& inOpts );
std::string mainGain( std::vector<std::string> &paths );



#define M3G_ERR_CANT_MODIFY_FILE -1
#define M3G_ERR_CANT_MAKE_TMP -2
#define M3G_ERR_NOT_ENOUGH_TMP_SPACE -3
#define M3G_ERR_RENAME_TMP -4
#define M3G_ERR_FILEOPEN   -5
#define M3G_ERR_READ       -6
#define M3G_ERR_WRITE      -7
#define M3G_ERR_TAGFORMAT  -8



}