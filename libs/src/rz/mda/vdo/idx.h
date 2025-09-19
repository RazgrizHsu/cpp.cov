#pragma once



namespace rz::mda::vdo {


static const int maxWidth = 960;

struct opts {

	bool rsz = false;

	int crf = 28;
	int mxw = maxWidth;
	string txt = "conv:";
};

//ffmpeg -i 輸入檔案.mov -c:v copy -c:a copy -map_metadata 0 輸出檔案.mp4
inline const string cmd_repack = R"(ffmpeg -i {} -c:v copy -c:a copy -map_metadata 0 -f mp4 {})";
inline optional<vector<string>> repackMp4( const fs::path &psrc, const fs::path &pout, opts opt = {} ) {

	auto pin = psrc.string();
	auto pou = pout;

	auto vi = mda::getVideoInfo( psrc );

	if ( fs::exists( pou ) ) fs::remove( pou );
	auto cmd = rz::fmt( cmd_repack, escapeShell( pin ), escapeShell( pou ) );

	//lg::info( "cmd: [{}]", cmd );

	return mda::cmdConv( cmd, opt.txt );
}



// size
// -vf scale={}:-2,setsar=1:1 //w大於max
// -vf scale=-2:{},setsar=1:1 //h大於max

// mac
//ffmpeg -i {} -progress pipe:2 -c:v libx265 {size} -vtag hvc1

//linux qua=28
//ffmpeg -hwaccel auto -i {} -progress pipe:2 -c:v hevc_nvenc -rc vbr -cq {qua} -profile:v main10 -pix_fmt p010le -b:v 0K {asize} -vtag hvc1

inline const string cmd_mac = R"(ffmpeg -i {} -c:v libx265 {} -crf {} -f mp4 {})";
inline const string cmd_lix = R"(ffmpeg -hwaccel auto -i {} -progress pipe:2 -c:v hevc_nvenc -rc vbr -profile:v main10 -pix_fmt p010le -b:v 0K {} -cq {} -vtag hvc1 -threads 4 -c:a aac -b:a 192k -map_metadata 0 -map_chapters 0 -movflags use_metadata_tags -f mp4 {})";


inline optional<vector<string>> convH265( const fs::path &psrc, const fs::path &pout, opts opt = {} ) {

	auto pin = psrc.string();
	auto pou = pout;

	auto vi = mda::getVideoInfo( psrc );

	string asize = "";
	if ( opt.rsz ) {
		if ( vi.w > opt.mxw || vi.h > opt.mxw ) {
			if ( vi.w > vi.h ) {
				if ( vi.w > opt.mxw ) asize = rz::fmt( "-vf scale={}:-2,setsar=1:1", opt.mxw );
			}
			else {
				if ( vi.h > opt.mxw ) asize = rz::fmt( "-vf scale=-2:{},setsar=1:1", opt.mxw );
			}

			lg::info( "[convH265] q[{}] o[{}x{}] -> to[{}]", opt.crf, vi.w, vi.h, opt.mxw );
		}
		else {
			lg::info( "[convH265] rsz[{}] q[{}] o[{}x{}]", opt.rsz, opt.crf, vi.w, vi.h );
		}
	}
	else {
		lg::info( "[convH265] rsz[{}] q[{}] o[{}x{}]", opt.rsz, opt.crf, vi.w, vi.h );
	}



	if ( fs::exists( pou ) ) fs::remove( pou );
#ifdef __APPLE__
	auto cmd = rz::fmt( cmd_mac, escapeShell( pin ), asize, opt.crf, escapeShell( pou ) );
#else
	auto cmd = rz::fmt( cmd_lix, escapeShell( pin ), asize, opt.crf, escapeShell( pou ) );
#endif


	lg::info( "cmd: [{}]", cmd );

	return mda::cmdConv( cmd, opt.txt );
}

inline optional<vector<string>> download( const string &url, const fs::path &pout, opts opt = {} ) {

	auto pin = url;
	auto pou = pout;

	if ( fs::exists( pou ) ) fs::remove( pou );
	auto cmd = rz::fmt( "ffmpeg -i \"{}\" -c copy {}", url, escapeShell( pou ) );


	//lg::info( "cmd: [{}]", cmd );

	return mda::cmdConv( cmd, opt.txt );
}

inline optional<vector<string>> convH265( const fs::path &psrc, opts opts = {} ) {
	auto pout = psrc.parent_path() / ( psrc.stem().string() + ".mp4" );
	return convH265( psrc, pout, opts );
}



}
