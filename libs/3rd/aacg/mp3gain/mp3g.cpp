#include <cstring>
#include <iostream>
#include <string>
#include <sstream>

extern "C" {
#include "apetag.h"
#include "id3tag.h"
#include "aacgain.h"
#include <utime.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mpglibDBL/interface.h"
#include "gain_analysis.h"
#include "rg_error.h"
}

#include <functional>
#include <filesystem>
#include "./mp3g.h"



namespace mp3g {

#define SWITCH_CHAR '-'
#define HEADERSIZE 4

#define CRC16_POLYNOMIAL        0x8005
#define BUFFERSIZE 3000000
#define WRITEBUFFERSIZE 100000

#define FULL_RECALC 1
#define AMP_RECALC 2
#define MIN_MAX_GAIN_RECALC 4



const gainOpts *opts = NULL;

typedef struct {
	unsigned long fileposition;
	unsigned char val[2];
} wbuffer;

/* Yes, yes, I know I should do something about these globals */

wbuffer writebuffer[WRITEBUFFERSIZE];

unsigned long writebuffercnt;

unsigned char buffer[BUFFERSIZE];

int writeself = 0;
int NowWriting = 0;
double lastfreq = -1.0;

int BadLayer = 0;
int LayerSet = 0;

int skipTag = 0;
int deleteTag = 0;
int forceRecalculateTag = 0;
int checkTagOnly = 0;
static int useId3 = 0;

int gSuccess;

long inbuffer;
unsigned long bitidx;
unsigned char *wrdpntr;
unsigned char *curframe;


FILE *inf = NULL;

FILE *outf;


unsigned long filepos;

static const double bitrate[4][16] = { { 1, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 1 }, { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, { 1, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 1 }, { 1, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 1 } };
static constexpr double frequency[4][4] = { { 11.025, 12, 8, 1 }, { 1, 1, 1, 1 }, { 22.05, 24, 16, 1 }, { 44.1, 48, 32, 1 } };

long arrbytesinframe[16];

/* instead of writing each byte change, I buffer them up */
static void flushWriteBuff() {
	for ( unsigned long i = 0; i < writebuffercnt; i++ ) {
		fseek( inf, writebuffer[i].fileposition,SEEK_SET );
		fwrite( writebuffer[i].val, 1, 2, inf );
	}
	writebuffercnt = 0;
};



static void addWriteBuff( unsigned long pos, unsigned char *vals ) {
	if ( writebuffercnt >= WRITEBUFFERSIZE ) {
		flushWriteBuff();
		fseek( inf, filepos,SEEK_SET );
	}
	writebuffer[writebuffercnt].fileposition = pos;
	writebuffer[writebuffercnt].val[0] = *vals;
	writebuffer[writebuffercnt].val[1] = vals[1];
	writebuffercnt++;
};


/* fill the mp3 buffer */
static unsigned long fillBuffer( long savelastbytes ) {
	unsigned long i;

	unsigned long skip = 0;
	if ( savelastbytes < 0 ) {
		skip = -savelastbytes;
		savelastbytes = 0;
	}

	if ( opts->useTmp && NowWriting ) { if ( fwrite( buffer, 1, inbuffer - savelastbytes, outf ) != static_cast<size_t>(inbuffer - savelastbytes) ) return 0; }

	if ( savelastbytes != 0 ) /* save some of the bytes at the end of the buffer */
		memmove( buffer, buffer+inbuffer-savelastbytes, savelastbytes );

	while ( skip > 0 ) {
		/* skip some bytes from the input file */
		unsigned long skipbuf = skip > BUFFERSIZE ? BUFFERSIZE : skip;

		i = fread( buffer, 1, skipbuf, inf );
		if ( i != skipbuf ) return 0;

		if ( opts->useTmp && NowWriting ) { if ( fwrite( buffer, 1, skipbuf, outf ) != skipbuf ) return 0; }
		filepos += i;
		skip -= skipbuf;
	}
	i = fread( buffer + savelastbytes, 1,BUFFERSIZE - savelastbytes, inf );

	filepos = filepos + i;
	inbuffer = i + savelastbytes;
	return i;
}


static const unsigned char maskLeft8bits[8] = { 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE };

static const unsigned char maskRight8bits[8] = { 0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01 };

static void set8Bits( unsigned short val ) {
	val <<= 8 - bitidx;
	wrdpntr[0] &= maskLeft8bits[bitidx];
	wrdpntr[0] |= val >> 8;
	wrdpntr[1] &= maskRight8bits[bitidx];
	wrdpntr[1] |= val & 0xFF;

	if ( !opts->useTmp ) addWriteBuff( filepos - ( inbuffer - ( wrdpntr - buffer ) ), wrdpntr );
}

static void skipBits( int nbits ) {
	bitidx += nbits;
	wrdpntr += bitidx >> 3;
	bitidx &= 7;
}

static unsigned char peek8Bits() {
	unsigned short rval = wrdpntr[0];
	rval <<= 8;
	rval |= wrdpntr[1];
	rval >>= 8 - bitidx;

	return rval & 0xFF;
}


template<typename... Args>
std::string format(const std::string& fmt, Args... args) {
	std::ostringstream result;
	const char* current = fmt.c_str();
	int index = 0;

	auto formatArg = [&](auto&& arg) {
		while (*current && *current != '{') {
			result << *current++;
		}
		if (*current == '{') {
			if (*(current + 1) == '}') {
				result << arg;
				current += 2;
			} else {
				throw std::runtime_error("Invalid format string");
			}
		}
		++index;
	};

	(formatArg(args), ...);

	while (*current) {
		if (*current == '{' || *current == '}') {
			throw std::runtime_error("Invalid format string");
		}
		result << *current++;
	}

	return result.str();
}


static unsigned long skipID3v2() {
	/*
	 *  An ID3v2 tag can be detected with the following pattern:
	 *    $49 44 33 yy yy xx zz zz zz zz
	 *  Where yy is less than $FF, xx is the 'flags' byte and zz is less than
	 *  $80.
	 */

	unsigned long ok = 1;

	if ( wrdpntr[0] == 'I' && wrdpntr[1] == 'D' && wrdpntr[2] == '3' && wrdpntr[3] < 0xFF && wrdpntr[4] < 0xFF ) {
		unsigned long ID3Size = static_cast<long>(wrdpntr[9]) | static_cast<long>(wrdpntr[8]) << 7 | static_cast<long>(wrdpntr[7]) << 14 | static_cast<long>(wrdpntr[6]) << 21;
		ID3Size += 10;

		wrdpntr = wrdpntr + ID3Size;

		if ( wrdpntr + HEADERSIZE - buffer > inbuffer ) {
			ok = fillBuffer( inbuffer - ( wrdpntr - buffer ) );
			wrdpntr = buffer;
		}
	}

	return ok;
}

static unsigned long __frameSearch( int startup, char *curfilename, std::stringstream &rst ) {
	static int startfreq;
	static int startmpegver;
	double bitbase;

	int done = 0;
	unsigned long ok = 1;

	if ( wrdpntr + HEADERSIZE - buffer > inbuffer ) {
		ok = fillBuffer( inbuffer - ( wrdpntr - buffer ) );
		wrdpntr = buffer;
		if ( !ok ) done = 1;
	}

	while ( !done ) {
		done = 1;

		if ( ( wrdpntr[0] & 0xFF ) != 0xFF ) done = 0;      /* first 8 bits must be '1' */
		else if ( ( wrdpntr[1] & 0xE0 ) != 0xE0 ) done = 0; /* next 3 bits are also '1' */
		else if ( ( wrdpntr[1] & 0x18 ) == 0x08 ) done = 0; /* invalid MPEG version */
		else if ( ( wrdpntr[2] & 0xF0 ) == 0xF0 ) done = 0; /* bad bitrate */
		else if ( ( wrdpntr[2] & 0xF0 ) == 0x00 ) done = 0; /* we'll just completely ignore "free format" bitrates */
		else if ( ( wrdpntr[2] & 0x0C ) == 0x0C ) done = 0; /* bad sample frequency */
		else if ( ( wrdpntr[1] & 0x06 ) != 0x02 ) {
			/* not Layer III */
			if ( !LayerSet ) {
				switch ( wrdpntr[1] & 0x06 ) {
					case 0x06: BadLayer = !false;
						rst << format( "ERR|{}|{}| is an MPEG Layer I file, not a layer III\n", curfilename, MP3GAIN_FILEFORMAT_NOTSUPPORTED );
						return 0;
					case 0x04: BadLayer = !false;
						rst << format( "ERR|{}|{}| is an MPEG Layer II file, not a layer III\n", curfilename, MP3GAIN_FILEFORMAT_NOTSUPPORTED );
						return 0;
				}
			}
			done = 0; /* probably just corrupt data, keep trying */
		}
		else if ( startup ) {
			startmpegver = wrdpntr[1] & 0x18;
			startfreq = wrdpntr[2] & 0x0C;
			long tempmpegver = startmpegver >> 3;
			if ( tempmpegver == 3 ) bitbase = 1152.0;
			else bitbase = 576.0;

			for ( int i = 0; i < 16; i++ ) arrbytesinframe[i] = static_cast<long>(floor( floor( bitbase * bitrate[tempmpegver][i] / frequency[tempmpegver][startfreq >> 2] ) / 8.0 ));
		}
		else {
			/* !startup -- if MPEG version or frequency is different,
										 then probably not correctly synched yet */
			if ( ( wrdpntr[1] & 0x18 ) != startmpegver ) done = 0;
			else if ( ( wrdpntr[2] & 0x0C ) != startfreq ) done = 0;
			else if ( ( wrdpntr[2] & 0xF0 ) == 0 ) /* bitrate is "free format" probably just
												  corrupt data if we've already found
												  valid frames */
				done = 0;
		}

		if ( !done ) wrdpntr++;

		if ( wrdpntr + HEADERSIZE - buffer > inbuffer ) {
			ok = fillBuffer( inbuffer - ( wrdpntr - buffer ) );
			wrdpntr = buffer;
			if ( !ok ) done = 1;
		}
	}

	if ( ok ) {
		if ( inbuffer - ( wrdpntr - buffer ) < arrbytesinframe[wrdpntr[2] >> 4 & 0x0F] + ( wrdpntr[2] >> 1 & 0x01 ) ) {
			ok = fillBuffer( inbuffer - ( wrdpntr - buffer ) );
			wrdpntr = buffer;
		}
		bitidx = 0;
		curframe = wrdpntr;
	}
	return ok;
}

// typedef unsigned long (*IFnFindFrame)(int startup );
//
// static IFnFindFrame makeFindFrame( char *fileName ) {
//
// 	auto act = [&fileName]( int startup ) {
//
// 		return __frameSearch( startup, fileName );
// 	};
// 	return act;
// }

using IFnFindFrame = std::function<unsigned long( int )>;

static IFnFindFrame makeFindFrame( char *fileName, std::stringstream &rst ) {
	return [fileName,&rst]( int startup ) {
		return __frameSearch( startup, fileName, rst );
	};
}



static int crcUpdate( int value, int crc ) {
	value <<= 8;
	for ( int i = 0; i < 8; i++ ) {
		value <<= 1;
		crc <<= 1;

		if ( ( crc ^ value ) & 0x10000 ) crc ^= CRC16_POLYNOMIAL;
	}
	return crc;
}



static void crcWriteHeader( int headerlength, char *header ) {
	int crc = 0xffff; /* (jo) init crc16 for error_protection */

	crc = crcUpdate( reinterpret_cast<unsigned char *>(header)[2], crc );
	crc = crcUpdate( reinterpret_cast<unsigned char *>(header)[3], crc );
	for ( int i = 6; i < headerlength; i++ ) { crc = crcUpdate( reinterpret_cast<unsigned char *>(header)[i], crc ); }

	header[4] = crc >> 8;
	header[5] = crc & 255;
}


static long getSizeOfFile( char *filename ) {
	long size = 0;

	if ( FILE *file = fopen( filename, "rb" ) ) {
		fseek( file, 0, SEEK_END );
		size = ftell( file );
		fclose( file );
	}

	return size;
}


int fileDel( char *filename ) { return remove( filename ); }

int fileMov( char *currentfilename, char *newfilename ) { return rename( currentfilename, newfilename ); }



/* Get File size and datetime stamp */
void setfileTime( char *filename, timeAction action ) {
	static int timeSaved = 0;
	static struct stat savedAttributes;

	if ( action == storeTime ) { timeSaved = stat( filename, &savedAttributes ) == 0; }
	else {
		if ( timeSaved ) {
			utimbuf setTime;

			setTime.actime = savedAttributes.st_atime;
			setTime.modtime = savedAttributes.st_mtime;
			timeSaved = 0;

			utime( filename, &setTime );
		}
	}
}



void scanFrameGain() {
	int ch;
	int gain;

	int mpegver = curframe[1] >> 3 & 0x03;
	int crcflag = curframe[1] & 0x01;
	int mode = curframe[3] >> 6 & 0x03;
	int nchan = mode == 3 ? 1 : 2;

	if ( !crcflag ) wrdpntr = curframe + 6;
	else wrdpntr = curframe + 4;

	bitidx = 0;

	if ( mpegver == 3 ) {
		/* 9 bit main_data_begin */
		wrdpntr++;
		bitidx = 1;

		if ( mode == 3 ) skipBits( 5 ); /* private bits */
		else skipBits( 3 );             /* private bits */

		skipBits( nchan * 4 ); /* scfsi[ch][band] */
		for ( int gr = 0; gr < 2; gr++ ) {
			for ( ch = 0; ch < nchan; ch++ ) {
				skipBits( 21 );
				gain = peek8Bits();
				if ( *minGain > gain ) { *minGain = gain; }
				if ( *maxGain < gain ) { *maxGain = gain; }
				skipBits( 38 );
			}
		}
	}
	else {
		/* mpegver != 3 */
		wrdpntr++; /* 8 bit main_data_begin */

		if ( mode == 3 ) skipBits( 1 );
		else skipBits( 2 );

		/* only one granule, so no loop */
		for ( ch = 0; ch < nchan; ch++ ) {
			skipBits( 21 );
			gain = peek8Bits();
			if ( *minGain > gain ) { *minGain = gain; }
			if ( *maxGain < gain ) { *maxGain = gain; }
			skipBits( 42 );
		}
	}
}


typedef unsigned long ( *IFnReport )( unsigned long percent, unsigned long bytes );
//using IFnReport = std::function<unsigned long(unsigned long percent, unsigned long bytes)>;

class LambdaWrapper {
public:
	template<typename F>
	LambdaWrapper( F &&lambda ) : m_lambda( std::forward<F>( lambda ) ) {}

	static unsigned long callback( unsigned long percent, unsigned long bytes ) {
		return instance->m_lambda( percent, bytes );
	}

	static std::unique_ptr<LambdaWrapper> instance;

private:
	std::function<unsigned long( unsigned long, unsigned long )> m_lambda;
};

std::unique_ptr<LambdaWrapper> LambdaWrapper::instance;

// Function to convert lambda to C-style function pointer
IFnReport lambdaToFunctionPointer( std::function<unsigned long( unsigned long, unsigned long )> lambda ) {
	LambdaWrapper::instance = std::make_unique<LambdaWrapper>( std::move( lambda ) );
	return &LambdaWrapper::callback;
}


#define EXPAND(...) __VA_ARGS__
#define RSTERR(...) rst << format( EXPAND(__VA_ARGS__) )

int changeGain( char *filename, AACGainHandle aacH, int leftgainchange, int rightgainchange, std::stringstream &rst, IFnReport reporter, IFnFindFrame frameSearch ) {
	long gFilesize = 0;
	int gainchange[2];
	long outlength; /* size checker when using Temp files */

	char *outfilename = NULL;
	BadLayer = 0;
	LayerSet = opts->reckless;

	NowWriting = !false;

	if ( leftgainchange == 0 && rightgainchange == 0 ) return 0;
	if ( aacH ) {
		int rc = aac_modify_gain( aacH, leftgainchange, rightgainchange, opts->quiteMode ? NULL : reporter );
		NowWriting = 0;
		if ( rc )
			RSTERR( "ERR|{}|{}| failed to modify gain\n", filename, MP3GAIN_FILEFORMAT_NOTSUPPORTED );
		return rc;
	}

	gainchange[0] = leftgainchange;
	gainchange[1] = rightgainchange;
	int singlechannel = leftgainchange != rightgainchange;

	if ( opts->keepOrigDT ) setfileTime( filename, storeTime );

	gFilesize = getSizeOfFile( filename );

	if ( opts->useTmp ) {

		outlength = static_cast<long>(strlen( filename ));
		outfilename = static_cast<char *>(malloc( outlength + 5 ));
		strcpy( outfilename, filename );
		if ( ( filename[outlength - 3] == 'T' || filename[outlength - 3] == 't' ) && ( filename[outlength - 2] == 'M' || filename[outlength - 2] == 'm' ) && ( filename[outlength - 1] == 'P' || filename[outlength - 1] == 'p' ) ) { strcat( outfilename, ".TMP" ); }
		else {
			outfilename[outlength - 3] = 'T';
			outfilename[outlength - 2] = 'M';
			outfilename[outlength - 1] = 'P';
		}

		inf = fopen( filename, "r+b" );

		if ( inf != NULL ) {
			outf = fopen( outfilename, "wb" );

			if ( outf == NULL ) {
				fclose( inf );
				inf = NULL;
				RSTERR( "ERR|{}|{}| Cannot open {} for tmp\n", filename, MP3GAIN_UNSPECIFED_ERROR, outfilename );
				return M3G_ERR_CANT_MAKE_TMP;
			}
		}
	}
	else { inf = fopen( filename, "r+b" ); }

	if ( inf == NULL ) {
		if ( opts->useTmp && outf != NULL ) fclose( outf );
		RSTERR( "ERR|{}|{}| Cannot open for modifying\n", filename, MP3GAIN_UNSPECIFED_ERROR );
		return M3G_ERR_CANT_MODIFY_FILE;
	}

	writebuffercnt = 0;
	inbuffer = 0;
	filepos = 0;
	bitidx = 0;
	if ( unsigned long ok = fillBuffer( 0 ) ) {
		unsigned long frame = 0;
		int mpegver;
		long bytesinframe;
		int bitridx;
		unsigned char gain;
		int ch;
		int mode;
		int freqidx;
		wrdpntr = buffer;

		ok = skipID3v2();

		ok = frameSearch( !false );
		if ( !ok ) {
			if ( !BadLayer )
				RSTERR( "ERR|{}|{}| Can't find any valid MP3 frames in file\n", filename, MP3GAIN_UNSPECIFED_ERROR );
		}
		else {
			int sideinfo_len;
			LayerSet = 1; /* We've found at least one valid layer 3 frame.
						   * Assume any later layer 1 or 2 frames are just
						   * bitstream corruption
						   */
			mode = curframe[3] >> 6 & 3;

			if ( ( curframe[1] & 0x08 ) == 0x08 ) /* MPEG 1 */
				sideinfo_len = mode == 3 ? 4 + 17 : 4 + 32;
			else /* MPEG 2 */
				sideinfo_len = mode == 3 ? 4 + 9 : 4 + 17;

			if ( !( curframe[1] & 0x01 ) ) sideinfo_len += 2;

			unsigned char *Xingcheck = curframe + sideinfo_len;

			//LAME CBR files have "Info" tags, not "Xing" tags
			if ( ( Xingcheck[0] == 'X' && Xingcheck[1] == 'i' && Xingcheck[2] == 'n' && Xingcheck[3] == 'g' ) || ( Xingcheck[0] == 'I' && Xingcheck[1] == 'n' && Xingcheck[2] == 'f' && Xingcheck[3] == 'o' ) ) {
				bitridx = curframe[2] >> 4 & 0x0F;
				if ( bitridx == 0 ) {
					RSTERR( "ERR|{}|{}| is free format (not currently supported)\n", filename, MP3GAIN_FILEFORMAT_NOTSUPPORTED );
					ok = 0;
				}
				else {
					mpegver = curframe[1] >> 3 & 0x03;
					freqidx = curframe[2] >> 2 & 0x03;

					bytesinframe = arrbytesinframe[bitridx] + ( curframe[2] >> 1 & 0x01 );

					wrdpntr = curframe + bytesinframe;

					ok = frameSearch( 0 );
				}
			}

			frame = 1;
		} /* if (!ok) else */

		while ( ok ) {
			bitridx = curframe[2] >> 4 & 0x0F;
			if ( singlechannel ) {
				if ( curframe[3] >> 6 & 0x01 ) {
					/* if mode is NOT stereo or dual channel */
					RSTERR( "ERR|{}|{}| Can't adjust single channel for mono or joint stereo\n", filename, MP3GAIN_FILEFORMAT_NOTSUPPORTED );
					ok = 0;
				}
			}
			if ( bitridx == 0 ) {
				RSTERR( "ERR|{}|{}| is free format (not currently supported)\n", filename, MP3GAIN_FILEFORMAT_NOTSUPPORTED );
				ok = 0;
			}
			if ( ok ) {
				mpegver = curframe[1] >> 3 & 0x03;
				int crcflag = curframe[1] & 0x01;
				freqidx = curframe[2] >> 2 & 0x03;

				bytesinframe = arrbytesinframe[bitridx] + ( curframe[2] >> 1 & 0x01 );
				mode = curframe[3] >> 6 & 0x03;
				int nchan = mode == 3 ? 1 : 2;

				if ( !crcflag )              /* we DO have a crc field */
					wrdpntr = curframe + 6;  /* 4-byte header, 2-byte CRC */
				else wrdpntr = curframe + 4; /* 4-byte header */

				bitidx = 0;

				if ( mpegver == 3 ) {
					/* 9 bit main_data_begin */
					wrdpntr++;
					bitidx = 1;

					if ( mode == 3 ) skipBits( 5 ); /* private bits */
					else skipBits( 3 );             /* private bits */

					skipBits( nchan * 4 ); /* scfsi[ch][band] */
					for ( int gr = 0; gr < 2; gr++ )
						for ( ch = 0; ch < nchan; ch++ ) {
							skipBits( 21 );
							gain = peek8Bits();
							if ( opts->warpGain ) gain += static_cast<unsigned char>(gainchange[ch]);
							else {
								if ( gain != 0 ) {
									if ( static_cast<int>(gain) + gainchange[ch] > 255 ) gain = 255;
									else if ( static_cast<int>(gain) + gainchange[ch] < 0 ) gain = 0;
									else gain += static_cast<unsigned char>(gainchange[ch]);
								}
							}
							set8Bits( gain );
							skipBits( 38 );
						}
					if ( !crcflag ) {
						if ( nchan == 1 ) crcWriteHeader( 23, reinterpret_cast<char *>(curframe) );
						else crcWriteHeader( 38, reinterpret_cast<char *>(curframe) );
						/* WRITETOFILE */
						if ( !opts->useTmp ) addWriteBuff( filepos - ( inbuffer - ( curframe + 4 - buffer ) ), curframe + 4 );
					}
				}
				else {
					/* mpegver != 3 */
					wrdpntr++; /* 8 bit main_data_begin */

					if ( mode == 3 ) skipBits( 1 );
					else skipBits( 2 );

					/* only one granule, so no loop */
					for ( ch = 0; ch < nchan; ch++ ) {
						skipBits( 21 );
						gain = peek8Bits();
						if ( opts->warpGain ) gain += static_cast<unsigned char>(gainchange[ch]);
						else {
							if ( gain != 0 ) {
								if ( static_cast<int>(gain) + gainchange[ch] > 255 ) gain = 255;
								else if ( static_cast<int>(gain) + gainchange[ch] < 0 ) gain = 0;
								else gain += static_cast<unsigned char>(gainchange[ch]);
							}
						}
						set8Bits( gain );
						skipBits( 42 );
					}
					if ( !crcflag ) {
						if ( nchan == 1 ) crcWriteHeader( 15, (char *)curframe );
						else crcWriteHeader( 23, (char *)curframe );
						/* WRITETOFILE */
						if ( !opts->useTmp ) addWriteBuff( filepos - ( inbuffer - ( curframe + 4 - buffer ) ), curframe + 4 );
					}
				}
				if ( !opts->quiteMode ) {
					frame++;
					if ( frame % 200 == 0 ) {
						ok = reporter( static_cast<unsigned long>((double)( filepos - ( inbuffer - ( curframe + bytesinframe - buffer ) ) ) * 100.0 / gFilesize), gFilesize );
						if ( !ok ) return ok;
					}
				}
				wrdpntr = curframe + bytesinframe;
				ok = frameSearch( 0 );
			}
		}
	}

	// if ( !opts->quiteMode ) { fprintf( std err, "                                                   \r" ); }

	if ( opts->useTmp ) {
		while ( fillBuffer( 0 ) );
		fflush( outf );
		fseek( outf, 0, SEEK_END );
		fseek( inf, 0, SEEK_END );
		outlength = ftell( outf );
		long inlength = ftell( inf );
		fclose( outf );
		fclose( inf );
		inf = NULL;

		if ( outlength != inlength ) {
			fileDel( outfilename );
			RSTERR( "ERR|{}|{}| Not enough temp space on disk to modify, Either free some space, or do not use \"temp file\" option\n", filename, MP3GAIN_UNSPECIFED_ERROR );
			return M3G_ERR_NOT_ENOUGH_TMP_SPACE;
		}
		if ( fileDel( filename ) ) {
			fileDel( outfilename ); //try to delete tmp file
			RSTERR( "ERR|{}|{}| Can't open for modifying\n", filename, MP3GAIN_UNSPECIFED_ERROR );
			return M3G_ERR_CANT_MODIFY_FILE;
		}
		if ( fileMov( outfilename, filename ) ) {
			RSTERR( "ERR|{}|{}| Problem re-naming {} to original path, The mp3 was correctly modified, but you will need to re-name\n", filename, MP3GAIN_UNSPECIFED_ERROR, outfilename );
			return M3G_ERR_RENAME_TMP;
		};
		if ( opts->keepOrigDT ) setfileTime( filename, setStoredTime );
		free( outfilename );
	}
	else {
		flushWriteBuff();
		fclose( inf );
		inf = NULL;
		if ( opts->keepOrigDT ) setfileTime( filename, setStoredTime );
	}

	NowWriting = 0;

	return 0;
}



void WriteAacGainTags( AACGainHandle aacH, MP3GainTagInfo *info ) {
	if ( info->haveAlbumGain ) aac_set_tag_float( aacH, replaygain_album_gain, info->albumGain );
	if ( info->haveAlbumPeak ) aac_set_tag_float( aacH, replaygain_album_peak, info->albumPeak );
	if ( info->haveAlbumMinMaxGain ) aac_set_tag_int_2( aacH, replaygain_album_minmax, info->albumMinGain, info->albumMaxGain );
	if ( info->haveTrackGain ) aac_set_tag_float( aacH, replaygain_track_gain, info->trackGain );
	if ( info->haveTrackPeak ) aac_set_tag_float( aacH, replaygain_track_peak, info->trackPeak );
	if ( info->haveMinMaxGain ) aac_set_tag_int_2( aacH, replaygain_track_minmax, info->minGain, info->maxGain );
	if ( info->haveUndo ) aac_set_tag_int_2( aacH, replaygain_undo, info->undoLeft, info->undoRight );
}


static void WriteMP3GainTag( char *filename, AACGainHandle aacH, MP3GainTagInfo *info, FileTagsStruct *fileTags, int saveTimeStamp ) {
	if ( aacH ) { WriteAacGainTags( aacH, info ); }
	else if ( useId3 ) {
		/* Write ID3 tag; remove stale APE tag if it exists. */
		if ( WriteMP3GainID3Tag( filename, info, saveTimeStamp ) >= 0 ) RemoveMP3GainAPETag( filename, saveTimeStamp );
	}
	else {
		/* Write APE tag */
		WriteMP3GainAPETag( filename, info, fileTags, saveTimeStamp );
	}
}


void changeGainAndTag(
	char *filename, AACGainHandle aacH, int leftgainchange, int rightgainchange,
	MP3GainTagInfo *tag, FileTagsStruct *fileTag, std::stringstream &rst, IFnReport reporter, IFnFindFrame frameSearch
) {
	if ( leftgainchange != 0 || rightgainchange != 0 ) {
		if ( !changeGain( filename, aacH, leftgainchange, rightgainchange, rst, reporter, frameSearch ) ) {
			if ( !tag->haveUndo ) {
				tag->undoLeft = 0;
				tag->undoRight = 0;
			}
			tag->dirty = !false;
			tag->undoRight -= rightgainchange;
			tag->undoLeft -= leftgainchange;
			tag->undoWrap = opts->warpGain;

			/* if undo == 0, then remove Undo tag */
			tag->haveUndo = !false;
			/* on second thought, don't remove it. Shortening the tag causes full file copy, which is slow so we avoid it if we can
					tag->haveUndo =
						((tag->undoRight != 0) ||
						 (tag->undoLeft != 0));
			*/

			if ( leftgainchange == rightgainchange ) {
				int curMax;
				int curMin;
				/* don't screw around with other fields if mis-matched left/right */
				double dblGainChange = leftgainchange * 1.505; /* approx. 5 * log10(2) */
				if ( tag->haveTrackGain ) { tag->trackGain -= dblGainChange; }
				if ( tag->haveTrackPeak ) { tag->trackPeak *= pow( 2.0, static_cast<double>(leftgainchange) / 4.0 ); }
				if ( tag->haveAlbumGain ) { tag->albumGain -= dblGainChange; }
				if ( tag->haveAlbumPeak ) { tag->albumPeak *= pow( 2.0, static_cast<double>(leftgainchange) / 4.0 ); }
				if ( tag->haveMinMaxGain ) {
					curMin = tag->minGain;
					curMax = tag->maxGain;
					curMin += leftgainchange;
					curMax += leftgainchange;
					if ( opts->warpGain ) {
						if ( curMin < 0 || curMin > 255 || curMax < 0 || curMax > 255 ) {
							/* we've lost the "real" min or max because of wrapping */
							tag->haveMinMaxGain = 0;
						}
					}
					else {
						tag->minGain = tag->minGain == 0 ? 0 : curMin < 0 ? 0 : curMin > 255 ? 255 : curMin;
						tag->maxGain = curMax < 0 ? 0 : curMax > 255 ? 255 : curMax;
					}
				}
				if ( tag->haveAlbumMinMaxGain ) {
					curMin = tag->albumMinGain;
					curMax = tag->albumMaxGain;
					curMin += leftgainchange;
					curMax += leftgainchange;
					if ( opts->warpGain ) {
						if ( curMin < 0 || curMin > 255 || curMax < 0 || curMax > 255 ) {
							/* we've lost the "real" min or max because of wrapping */
							tag->haveAlbumMinMaxGain = 0;
						}
					}
					else {
						tag->albumMinGain = tag->albumMinGain == 0 ? 0 : curMin < 0 ? 0 : curMin > 255 ? 255 : curMin;
						tag->albumMaxGain = curMax < 0 ? 0 : curMax > 255 ? 255 : curMax;
					}
				}
			} // if (leftgainchange == rightgainchange ...
			WriteMP3GainTag( filename, aacH, tag, fileTag, opts->keepOrigDT );
		} // if (!changeGain(filename ...
	}     // if (leftgainchange !=0 ...
}



void ReadAacTags( AACGainHandle gh, MP3GainTagInfo *info ) {
	int p1, p2;

	if ( aac_get_tag_float( gh, replaygain_album_gain, &info->albumGain ) == 0 ) info->haveAlbumGain = !false;
	if ( aac_get_tag_float( gh, replaygain_album_peak, &info->albumPeak ) == 0 ) info->haveAlbumPeak = !false;
	if ( aac_get_tag_int_2( gh, replaygain_album_minmax, &p1, &p2 ) == 0 ) {
		info->albumMinGain = p1;
		info->albumMaxGain = p2;
		info->haveAlbumMinMaxGain = !false;
	}
	if ( aac_get_tag_float( gh, replaygain_track_gain, &info->trackGain ) == 0 ) info->haveTrackGain = !false;
	if ( aac_get_tag_float( gh, replaygain_track_peak, &info->trackPeak ) == 0 ) info->haveTrackPeak = !false;
	if ( aac_get_tag_int_2( gh, replaygain_track_minmax, &p1, &p2 ) == 0 ) {
		info->minGain = p1;
		info->maxGain = p2;
		info->haveMinMaxGain = !false;
	}
	if ( aac_get_tag_int_2( gh, replaygain_undo, &p1, &p2 ) == 0 ) {
		info->undoLeft = p1;
		info->undoRight = p2;
		info->haveUndo = !false;
	}
}

void dumpTaginfo( MP3GainTagInfo *info ) {
	fprintf( stderr, "haveAlbumGain       %d  albumGain %f\n", info->haveAlbumGain, info->albumGain );
	fprintf( stderr, "haveAlbumPeak       %d  albumPeak %f\n", info->haveAlbumPeak, info->albumPeak );
	fprintf( stderr, "haveAlbumMinMaxGain %d  min %d  max %d\n", info->haveAlbumMinMaxGain, info->albumMinGain, info->albumMaxGain );
	fprintf( stderr, "haveTrackGain       %d  trackGain %f\n", info->haveTrackGain, info->trackGain );
	fprintf( stderr, "haveTrackPeak       %d  trackPeak %f\n", info->haveTrackPeak, info->trackPeak );
	fprintf( stderr, "haveMinMaxGain      %d  min %d  max %d\n", info->haveMinMaxGain, info->minGain, info->maxGain );
}


// Usage: appName [options] <infile> [<infile 2> ...]
// options:
// 	-v - show version number
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

#define BUFFERSIZE 3000000
#define HEADERSIZE 4


#define _GETFNAME(x) const_cast<char*>(paths[x].c_str())
#define RST(...) rst << format(__VA_ARGS__)

int numFiles, totFiles;

std::string mainGain( std::vector<std::string> &paths, const gainOpts &inOpts ) {

	std::stringstream rst;


	opts = &inOpts;


	auto repoAnalyzed = []( unsigned long percent, unsigned long bytes ) -> unsigned long {
		//char fileDivFiles[21];
		//fileDivFiles[0] = '\0';
		//if ( totFiles - 1 ) sprintf( fileDivFiles, "[%d/%d]", numFiles, totFiles ); // if 1 file then don't show [x/n]
		//fprintf( std err, "%s %2lu%% of %lu bytes analyzed\r", fileDivFiles, percent, bytes );

		if ( opts->onProgress ) opts->onProgress( numFiles, percent, bytes );
		return 1;
	};

	auto queryUserForClipping = [&rst]( char *argv_mainloop, int intGainChange ) {

		RST( "WARNING! may clip with mp3 gain change {}\n", argv_mainloop, intGainChange );
		return 1;
	};



	MPSTR mp;
	unsigned long ok;
	int mode;
	unsigned char *Xingcheck;
	unsigned long frame;
	int nchan;
	int bitridx;
	int freqidx;
	long bytesinframe;
	double dBchange;
	double dblGainChange;
	int intGainChange = 0;
	int intAlbumGainChange = 0;
	int nprocsamp;
	int first = 1;
	int fileIdx;
	Float_t maxsample;
	unsigned char maxgain;
	unsigned char mingain;
	char isAnalysError = 0;
	int *fileok;
	int goAhead;
	int mpegver;
	int sideinfo_len;
	long gFilesize = 0;
	int decodeSuccess;
	MP3GainTagInfo *tagInfo;
	MP3GainTagInfo *curTag;
	FileTagsStruct *fileTags;
	int albumRecalc;
	double curAlbumGain = 0;
	double curAlbumPeak = 0;
	unsigned char curAlbumMinGain = 0;
	unsigned char curAlbumMaxGain = 0;

	AACGainHandle *aacInfo;
	gSuccess = 1;

	maxAmpOnly = opts->onlyMaxAmp;
	numFiles = 0;


	//這邊改有點複雜, 暫時用有值再指定
	if ( opts->s ) {
		checkTagOnly = opts->s->checkTagOnly;
		deleteTag = opts->s->deleteTag;
		skipTag = opts->s->skipTag;
		forceRecalculateTag = opts->s->forceRecalculateTag;
		useId3 = opts->s->useId3;
	}



	totFiles = paths.size();

	fileok = static_cast<int *>(malloc( sizeof( int ) * totFiles ));
	tagInfo = static_cast<MP3GainTagInfo *>(calloc( totFiles, sizeof( MP3GainTagInfo ) ));
	fileTags = static_cast<FileTagsStruct *>(malloc( sizeof( FileTagsStruct ) * totFiles ));

	aacInfo = static_cast<AACGainHandle *>(malloc( sizeof( AACGainHandle ) * totFiles ));

	if ( opts->dbFormat ) {
		if ( checkTagOnly ) { RST( "File\tMP3 gain\tdB gain\tMax Amplitude\tMax global_gain\tMin global_gain\tAlbum gain\tAlbum dB gain\tAlbum Max Amplitude\tAlbum Max global_gain\tAlbum Min global_gain\n" ); }
		else if ( opts->undoChanges ) { RST( "File\tleft global_gain change\tright global_gain change\n" ); }
		else { RST( "File\tMP3 gain\tdB gain\tMax Amplitude\tMax global_gain\tMin global_gain\n" ); }

	}

	/* read all the tags first */

	for ( fileIdx = 0; fileIdx < totFiles; fileIdx++ ) {
		fileok[fileIdx] = 0;
		auto curfilename = _GETFNAME( fileIdx );

		fileTags[fileIdx].apeTag = NULL;
		fileTags[fileIdx].lyrics3tag = NULL;
		fileTags[fileIdx].id31tag = NULL;
		tagInfo[fileIdx].dirty = 0;
		tagInfo[fileIdx].haveAlbumGain = 0;
		tagInfo[fileIdx].haveAlbumPeak = 0;
		tagInfo[fileIdx].haveTrackGain = 0;
		tagInfo[fileIdx].haveTrackPeak = 0;
		tagInfo[fileIdx].haveUndo = 0;
		tagInfo[fileIdx].haveMinMaxGain = 0;
		tagInfo[fileIdx].haveAlbumMinMaxGain = 0;
		tagInfo[fileIdx].recalc = 0;

		//check for aac file; open it if found, (( we try to open aac even if /f (reckless)
		if ( aac_open( curfilename, opts->useTmp, opts->keepOrigDT, opts->trackIndex, &aacInfo[fileIdx] ) != 0 ) {
			//in case of any errors, don't continue processing so there is no risk of corrupting a bad file
			RSTERR( "ERR|{}|{}| is not a valid mp4/m4a file.\n", curfilename, MP3GAIN_FILEFORMAT_NOTSUPPORTED );
			exit( 1 );
		}

		if ( !skipTag && !deleteTag ) {
			if ( aacInfo[fileIdx] ) { if ( !skipTag ) ReadAacTags( aacInfo[fileIdx], &tagInfo[fileIdx] ); }
			else {
				ReadMP3GainAPETag( curfilename, &tagInfo[fileIdx], &fileTags[fileIdx] );
				if ( useId3 ) {
					if ( tagInfo[fileIdx].haveTrackGain || tagInfo[fileIdx].haveAlbumGain || tagInfo[fileIdx].haveMinMaxGain || tagInfo[fileIdx].haveAlbumMinMaxGain || tagInfo[fileIdx].haveUndo ) {
						/* Mark the file dirty to force upgrade to ID3v2 */
						tagInfo[fileIdx].dirty = 1;
					}
					ReadMP3GainID3Tag( curfilename, &tagInfo[fileIdx] );
				}
			}
			/*fprintf(stdout,"Read previous tags from %s\n",curfilename);
			  dumpTaginfo(&(tagInfo[fileIdx]));*/
			if ( forceRecalculateTag ) {
				if ( tagInfo[fileIdx].haveAlbumGain ) {
					tagInfo[fileIdx].dirty = !false;
					tagInfo[fileIdx].haveAlbumGain = 0;
				}
				if ( tagInfo[fileIdx].haveAlbumPeak ) {
					tagInfo[fileIdx].dirty = !false;
					tagInfo[fileIdx].haveAlbumPeak = 0;
				}
				if ( tagInfo[fileIdx].haveTrackGain ) {
					tagInfo[fileIdx].dirty = !false;
					tagInfo[fileIdx].haveTrackGain = 0;
				}
				if ( tagInfo[fileIdx].haveTrackPeak ) {
					tagInfo[fileIdx].dirty = !false;
					tagInfo[fileIdx].haveTrackPeak = 0;
				}
				if ( tagInfo[fileIdx].haveMinMaxGain ) {
					tagInfo[fileIdx].dirty = !false;
					tagInfo[fileIdx].haveMinMaxGain = 0;
				}
				if ( tagInfo[fileIdx].haveAlbumMinMaxGain ) {
					tagInfo[fileIdx].dirty = !false;
					tagInfo[fileIdx].haveAlbumMinMaxGain = 0;
				}
			}
		}
	}

	albumRecalc = forceRecalculateTag || skipTag ? FULL_RECALC : 0;
	if ( !skipTag && !deleteTag && !forceRecalculateTag ) {
		if ( totFiles > 1 ) {
			curAlbumGain = tagInfo[0].albumGain;
			curAlbumPeak = tagInfo[0].albumPeak;
			curAlbumMinGain = tagInfo[0].albumMinGain;
			curAlbumMaxGain = tagInfo[0].albumMaxGain;
		}
		for ( fileIdx = 0; fileIdx < totFiles; fileIdx++ ) {
			if ( !maxAmpOnly ) {
				if ( totFiles > 1 && opts->applyType != ByTrack && !opts->anaTrack ) {
					if ( !tagInfo[fileIdx].haveAlbumGain ) { albumRecalc |= FULL_RECALC; }
					else if ( tagInfo[fileIdx].albumGain != curAlbumGain ) { albumRecalc |= FULL_RECALC; }
				}
				if ( !tagInfo[fileIdx].haveTrackGain ) { tagInfo[fileIdx].recalc |= FULL_RECALC; }
			}
			if ( totFiles > 1 && opts->applyType != ByTrack && !opts->anaTrack ) {
				if ( !tagInfo[fileIdx].haveAlbumPeak ) { albumRecalc |= AMP_RECALC; }
				else if ( tagInfo[fileIdx].albumPeak != curAlbumPeak ) { albumRecalc |= AMP_RECALC; }
				if ( !tagInfo[fileIdx].haveAlbumMinMaxGain ) { albumRecalc |= MIN_MAX_GAIN_RECALC; }
				else if ( tagInfo[fileIdx].albumMaxGain != curAlbumMaxGain ) { albumRecalc |= MIN_MAX_GAIN_RECALC; }
				else if ( tagInfo[fileIdx].albumMinGain != curAlbumMinGain ) { albumRecalc |= MIN_MAX_GAIN_RECALC; }
			}
			if ( !tagInfo[fileIdx].haveTrackPeak ) { tagInfo[fileIdx].recalc |= AMP_RECALC; }
			if ( !tagInfo[fileIdx].haveMinMaxGain ) { tagInfo[fileIdx].recalc |= MIN_MAX_GAIN_RECALC; }
		}
	}

	for ( fileIdx = 0; fileIdx < totFiles; fileIdx++ ) {
		AACGainHandle aacH = aacInfo[fileIdx];
		memset( &mp, 0, sizeof( mp ) );

		auto ireport = lambdaToFunctionPointer
		(
			[&]( unsigned long percent, unsigned long bytes ) -> unsigned long {
				//std::cerr << "[report] idx:" << numFiles << ", percent: " << percent << std::endl << std::flush;
				if ( opts->onProgress ) opts->onProgress( fileIdx, percent, bytes );
				return 1;
			}
		);

		// if the entire Album requires some kind of recalculation, then each track needs it
		tagInfo[fileIdx].recalc |= albumRecalc;

		auto curfilename = _GETFNAME( fileIdx );

		auto findFrame = makeFindFrame( curfilename, rst );

		if ( checkTagOnly ) {
			curTag = tagInfo + fileIdx;
			if ( curTag->haveTrackGain ) {
				dblGainChange = curTag->trackGain / ( 5.0 * log10( 2.0 ) );

				if ( fabs( dblGainChange ) - static_cast<double>((int)fabs( dblGainChange )) < 0.5 ) intGainChange = static_cast<int>(dblGainChange);
				else intGainChange = static_cast<int>(dblGainChange) + ( dblGainChange < 0 ? -1 : 1 );
			}
			if ( curTag->haveAlbumGain ) {
				dblGainChange = curTag->albumGain / ( 5.0 * log10( 2.0 ) );

				if ( fabs( dblGainChange ) - static_cast<double>((int)fabs( dblGainChange )) < 0.5 ) intAlbumGainChange = static_cast<int>(dblGainChange);
				else intAlbumGainChange = static_cast<int>(dblGainChange) + ( dblGainChange < 0 ? -1 : 1 );
			}
			if ( !opts->dbFormat ) {
				RST( "{}\n", curfilename );
				if ( curTag->haveTrackGain ) {
					RST( "Recommended \"Track\" dB change: {:.3f}\n", curTag->trackGain );
					RST( "Recommended \"Track\" mp3 gain change: {}\n", intGainChange );
					if ( curTag->haveTrackPeak ) { if ( curTag->trackPeak * pow( 2.0, static_cast<double>(intGainChange) / 4.0 ) > 1.0 ) { RST( "WARNING: some clipping may occur with this gain change!\n" ); } }
				}
				if ( curTag->haveTrackPeak )
					RST( "Max PCM sample at current gain: {:.3f}\n", curTag->trackPeak * 32768.0 );
				if ( curTag->haveMinMaxGain ) {
					RST( "Max mp3 global gain field: {}\n", curTag->maxGain );
					RST( "Min mp3 global gain field: {}\n", curTag->minGain );
				}
				if ( curTag->haveAlbumGain ) {
					RST( "Recommended \"Album\" dB change: {:.3f}\n", curTag->albumGain );
					RST( "Recommended \"Album\" mp3 gain change: {}\n", intAlbumGainChange );
					if ( curTag->haveTrackPeak ) { if ( curTag->trackPeak * pow( 2.0, static_cast<double>(intAlbumGainChange) / 4.0 ) > 1.0 ) { RST( "WARNING: some clipping may occur with this gain change!\n" ); } }
				}
				if ( curTag->haveAlbumPeak ) { RST( "Max Album PCM sample at current gain: {:.3f}\n", curTag->albumPeak * 32768.0 ); }
				if ( curTag->haveAlbumMinMaxGain ) {
					RST( "Max Album mp3 global gain field: {}\n", curTag->albumMaxGain );
					RST( "Min Album mp3 global gain field: {}\n", curTag->albumMinGain );
				}
				RST( "\n" );
			}
			else {
				RST( "{}\t", curfilename );
				if ( curTag->haveTrackGain ) {
					RST( "{}\t", intGainChange );
					RST( "{:.3f}\t", curTag->trackGain );
				}
				else { RST( "NA\tNA\t" ); }
				if ( curTag->haveTrackPeak ) { RST( "{:.3f}\t", curTag->trackPeak * 32768.0 ); }
				else { RST( "NA\t" ); }
				if ( curTag->haveMinMaxGain ) {
					RST( "{}\t", curTag->maxGain );
					RST( "{}\t", curTag->minGain );
				}
				else { RST( "NA\tNA\t" ); }
				if ( curTag->haveAlbumGain ) {
					RST( "{}\t", intAlbumGainChange );
					RST( "{:.3f}\t", curTag->albumGain );
				}
				else { RST( "NA\tNA\t" ); }
				if ( curTag->haveAlbumPeak ) { RST( "{:.3f}\t", curTag->albumPeak * 32768.0 ); }
				else { RST( "NA\t" ); }
				if ( curTag->haveAlbumMinMaxGain ) {
					RST( "{}\t", curTag->albumMaxGain );
					RST( "{}\n", curTag->albumMinGain );
				}
				else { RST( "NA\tNA\n" ); }

			}
		}
		else if ( opts->undoChanges ) {
			//directGain = !false; // so we don't write the tag a second time
			if ( tagInfo[fileIdx].haveUndo && ( tagInfo[fileIdx].undoLeft || tagInfo[fileIdx].undoRight ) ) {
				if ( !opts->quiteMode && !opts->dbFormat )
					RST( "ERR| Undoing mp3gain changes ({},{}) to {}...\n", tagInfo[fileIdx].undoLeft, tagInfo[fileIdx].undoRight, curfilename );

				if ( opts->dbFormat )
					RST( "{}\t{}\t{}\n", curfilename, tagInfo[fileIdx].undoLeft, tagInfo[fileIdx].undoRight );

				changeGainAndTag( curfilename, aacH, tagInfo[fileIdx].undoLeft, tagInfo[fileIdx].undoRight, tagInfo + fileIdx, fileTags + fileIdx, rst, ireport, findFrame );
			}
			else {
				if ( opts->dbFormat ) { RST( "{}\t0\t0\n", curfilename ); }
				else if ( !opts->quiteMode ) {
					if ( tagInfo[fileIdx].haveUndo ) { RST( "ERR|No changes to undo in {}\n", curfilename ); }
					else { RST( "ERR|No undo information in {}\n", curfilename ); }
				}
			}
		}
		else if ( opts->directChId ) {
			if ( !opts->quiteMode )
				RST( "ERR|Applying gain change of {} to CHANNEL {} of {}...\n", opts->directGain, (int)opts->directChId, curfilename );
			if ( opts->directChId == LEFT ) {
				/* do right channel */
				if ( skipTag ) { changeGain( curfilename, aacH, 0, opts->directGain, rst, ireport, findFrame ); }
				else { changeGainAndTag( curfilename, aacH, 0, opts->directGain, tagInfo + fileIdx, fileTags + fileIdx, rst, ireport, findFrame ); }
			}
			else {
				/* do left channel */
				if ( skipTag ) { changeGain( curfilename, aacH, opts->directGain, 0, rst, ireport, findFrame ); }
				else { changeGainAndTag( curfilename, aacH, opts->directGain, 0, tagInfo + fileIdx, fileTags + fileIdx, rst, ireport, findFrame ); }
			}
			if ( !opts->quiteMode && gSuccess == 1 )
				RST( "ERR|\ndone\n" );
		}
		else if ( opts->directGain ) {
			if ( !opts->quiteMode )
				RST( "ERR|Applying gain change of {} to {}...\n", opts->directGain, curfilename );
			if ( skipTag ) { changeGain( curfilename, aacH, opts->directGain, opts->directGain, rst, ireport, findFrame ); }
			else { changeGainAndTag( curfilename, aacH, opts->directGain, opts->directGain, tagInfo + fileIdx, fileTags + fileIdx, rst, ireport, findFrame ); }
			if ( !opts->quiteMode && gSuccess == 1 )
				RST( "ERR|\ndone\n" );
		}
		else if ( deleteTag ) {
			if ( aacH ) aac_clear_rg_tags( aacH );
			else {
				RemoveMP3GainAPETag( curfilename, opts->keepOrigDT );
				if ( useId3 ) { RemoveMP3GainID3Tag( curfilename, opts->keepOrigDT ); }
			}
			if ( !opts->quiteMode && !opts->dbFormat )
				RST( "ERR|Deleting tag info of {}...\n", curfilename );
			if ( opts->dbFormat )
				RST( "{}\tNA\tNA\tNA\tNA\tNA\n", curfilename );
		}
		else {
			if ( !opts->dbFormat )
				RST( "file: {}\n", curfilename );

			if ( tagInfo[fileIdx].recalc > 0 ) {
				gFilesize = getSizeOfFile( curfilename );
				if ( !aacH ) inf = fopen( curfilename, "rb" );
			}

			if ( !aacH && inf == NULL && tagInfo[fileIdx].recalc > 0 ) {
				RST( "Can't open {} for reading\n", curfilename );

			}
			else {
				if ( !aacH ) InitMP3( &mp );
				if ( tagInfo[fileIdx].recalc == 0 ) {
					maxsample = tagInfo[fileIdx].trackPeak * 32768.0;
					maxgain = tagInfo[fileIdx].maxGain;
					mingain = tagInfo[fileIdx].minGain;
					ok = !false;
				}
				else {
					if ( !( tagInfo[fileIdx].recalc & FULL_RECALC || tagInfo[fileIdx].recalc & AMP_RECALC ) ) {
						/* only min/max rescan */
						maxsample = tagInfo[fileIdx].trackPeak * 32768.0;
					}
					else { maxsample = 0; }
					if ( aacH ) {
						int rc;

						if ( first ) {
							lastfreq = aac_get_sample_rate( aacH );
							InitGainAnalysis( static_cast<long>(lastfreq) );
							isAnalysError = 0;
							first = 0;
						}
						else {
							if ( aac_get_sample_rate( aacH ) != lastfreq ) {
								lastfreq = aac_get_sample_rate( aacH );
								ResetSampleFrequency( static_cast<long>(lastfreq) );
							}
						}

						numFiles++;

						if ( maxAmpOnly ) rc = aac_compute_peak( aacH, &maxsample, &mingain, &maxgain, opts->quiteMode ? NULL : repoAnalyzed );
						else rc = aac_compute_gain( aacH, &maxsample, &mingain, &maxgain, opts->quiteMode ? NULL : repoAnalyzed );
						//in case of any error, bail to avoid corrupting file
						if ( rc != 0 ) {
							RSTERR( "ERR|{}|{}| is not a valid mp4/m4a file.\n", curfilename, MP3GAIN_FILEFORMAT_NOTSUPPORTED );
							exit( 1 );
						}
						ok = !false;
					}
					else {
						BadLayer = 0;
						LayerSet = opts->reckless;
						maxgain = 0;
						mingain = 255;
						inbuffer = 0;
						filepos = 0;
						bitidx = 0;
						ok = fillBuffer( 0 );
					}
				}
				if ( ok ) {
					if ( !aacH && tagInfo[fileIdx].recalc > 0 ) {
						wrdpntr = buffer;

						ok = skipID3v2();

						ok = findFrame( !false );
					}

					if ( !ok && !aacH ) {
						if ( !BadLayer ) {
							RST( "Can't find any valid MP3 frames in file {}\n", curfilename );

						}
					}
					else {
						LayerSet = 1; /* We've found at least one valid layer 3 frame.
								   * Assume any later layer 1 or 2 frames are just
								   * bitstream corruption
								   */
						fileok[fileIdx] = !false;
						if ( !aacH || tagInfo[fileIdx].recalc == 0 ) {
							numFiles++;
						}

						if ( !aacH && tagInfo[fileIdx].recalc > 0 ) {
							mode = curframe[3] >> 6 & 3;

							if ( ( curframe[1] & 0x08 ) == 0x08 ) /* MPEG 1 */
								sideinfo_len = ( curframe[3] & 0xC0 ) == 0xC0 ? 4 + 17 : 4 + 32;
							else /* MPEG 2 */
								sideinfo_len = ( curframe[3] & 0xC0 ) == 0xC0 ? 4 + 9 : 4 + 17;

							if ( !( curframe[1] & 0x01 ) ) sideinfo_len += 2;

							Xingcheck = curframe + sideinfo_len;
							//LAME CBR files have "Info" tags, not "Xing" tags
							if ( ( Xingcheck[0] == 'X' && Xingcheck[1] == 'i' && Xingcheck[2] == 'n' && Xingcheck[3] == 'g' ) || ( Xingcheck[0] == 'I' && Xingcheck[1] == 'n' && Xingcheck[2] == 'f' && Xingcheck[3] == 'o' ) ) {
								bitridx = curframe[2] >> 4 & 0x0F;
								if ( bitridx == 0 ) {
									RST( "{} is free format (not currently supported)\n", curfilename );

									ok = 0;
								}
								else {
									mpegver = curframe[1] >> 3 & 0x03;
									freqidx = curframe[2] >> 2 & 0x03;

									bytesinframe = arrbytesinframe[bitridx] + ( curframe[2] >> 1 & 0x01 );

									wrdpntr = curframe + bytesinframe;

									ok = findFrame( 0 );
								}
							}

							frame = 1;

							if ( !maxAmpOnly ) {
								if ( ok ) {
									mpegver = curframe[1] >> 3 & 0x03;
									freqidx = curframe[2] >> 2 & 0x03;

									if ( first ) {
										lastfreq = frequency[mpegver][freqidx];
										InitGainAnalysis( static_cast<long>(lastfreq * 1000.0) );
										isAnalysError = 0;
										first = 0;
									}
									else {
										if ( frequency[mpegver][freqidx] != lastfreq ) {
											lastfreq = frequency[mpegver][freqidx];
											ResetSampleFrequency( static_cast<long>(lastfreq * 1000.0) );
										}
									}
								}
							}
							else { isAnalysError = 0; }

							while ( ok ) {
								bitridx = curframe[2] >> 4 & 0x0F;
								if ( bitridx == 0 ) {
									RST( "{} is free format (not currently supported)\n", curfilename );

									ok = 0;
								}
								else {
									mpegver = curframe[1] >> 3 & 0x03;
									//int crcflag = curframe[1] & 0x01;
									freqidx = curframe[2] >> 2 & 0x03;

									bytesinframe = arrbytesinframe[bitridx] + ( curframe[2] >> 1 & 0x01 );
									mode = curframe[3] >> 6 & 0x03;
									nchan = mode == 3 ? 1 : 2;

									if ( inbuffer >= bytesinframe ) {
										Float_t rsamples[1152];
										Float_t lsamples[1152];
										lSamp = lsamples;
										rSamp = rsamples;
										maxSamp = &maxsample;
										maxGain = &maxgain;
										minGain = &mingain;
										procSamp = 0;
										if ( tagInfo[fileIdx].recalc & AMP_RECALC || tagInfo[fileIdx].recalc & FULL_RECALC ) { decodeSuccess = decodeMP3( &mp, curframe, bytesinframe, &nprocsamp ); }
										else {
											//don't need to actually decode frame, just scan for min/max gain values
											decodeSuccess = !MP3_OK;
											scanFrameGain(); //curframe);
										}
										if ( decodeSuccess == MP3_OK ) {
											if ( !maxAmpOnly && tagInfo[fileIdx].recalc & FULL_RECALC ) {
												if ( AnalyzeSamples( lsamples, rsamples, procSamp / nchan, nchan ) == GAIN_ANALYSIS_ERROR ) {
													RST( "ERR|Error analyzing further samples (max time reached)          \n" );
													isAnalysError = !false;
													ok = 0;
												}
											}
										}
									}


									if ( !isAnalysError ) {
										wrdpntr = curframe + bytesinframe;
										ok = findFrame( 0 );
									}

									if ( !opts->quiteMode ) { if ( !( ++frame % 200 ) ) { repoAnalyzed( static_cast<int>((double)( filepos - ( inbuffer - ( curframe + bytesinframe - buffer ) ) ) * 100.0 / gFilesize), gFilesize ); } }
								}
							}
						}

						//if ( !opts->quiteMode ) RST( "ERR------------------------------------" );

						if ( tagInfo[fileIdx].recalc & FULL_RECALC ) {
							if ( maxAmpOnly ) dBchange = 0;
							else dBchange = GetTitleGain();
						}
						else { dBchange = tagInfo[fileIdx].trackGain; }

						if ( dBchange == GAIN_NOT_ENOUGH_SAMPLES ) {
							RST( "Not enough samples in {} to do analysis\n", curfilename );

							numFiles--;
						}
						else {
							/* even if skipTag is on, we'll leave this part running just to store the minpeak and maxpeak */
							curTag = tagInfo + fileIdx;
							if ( !maxAmpOnly ) {
								if ( /* if we don't already have a tagged track gain OR we have it, but it doesn't match */
									!curTag->haveTrackGain || ( curTag->haveTrackGain && fabs( dBchange - curTag->trackGain ) >= 0.01 ) ) {
									curTag->dirty = !false;
									curTag->haveTrackGain = 1;
									curTag->trackGain = dBchange;
								}
							}
							if ( !curTag->haveMinMaxGain || /* if minGain or maxGain doesn't match tag */
								( curTag->haveMinMaxGain && ( curTag->minGain != mingain || curTag->maxGain != maxgain ) ) ) {
								curTag->dirty = !false;
								curTag->haveMinMaxGain = !false;
								curTag->minGain = mingain;
								curTag->maxGain = maxgain;
							}

							if ( !curTag->haveTrackPeak || ( curTag->haveTrackPeak && fabs( maxsample - curTag->trackPeak * 32768.0 ) >= 3.3 ) ) {
								curTag->dirty = !false;
								curTag->haveTrackPeak = !false;
								curTag->trackPeak = maxsample / 32768.0;
							}
							/* the TAG version of the suggested Track Gain should ALWAYS be based on the 89dB standard.
							   So we don't modify the suggested gain change until this point */
							dBchange += opts->dbGainMod;

							dblGainChange = dBchange / ( 5.0 * log10( 2.0 ) );

							if ( fabs( dblGainChange ) - static_cast<double>((int)fabs( dblGainChange )) < 0.5 ) intGainChange = static_cast<int>(dblGainChange);
							else intGainChange = static_cast<int>(dblGainChange) + ( dblGainChange < 0 ? -1 : 1 );
							intGainChange += opts->mp3GainMod;

							if ( opts->dbFormat ) {
								RST( "{}\t{}\t{:.3f}\t{:.3f}\t{}\t{}\n", curfilename, intGainChange, dBchange, maxsample, maxgain, mingain );
							}
							if ( opts->applyType != ByTrack && opts->applyType != ByAlbum ) {
								if ( !opts->dbFormat ) {
									RST( "Rec Track dB:{:.3f}\n", dBchange );
									RST( "Rec Track mp3 gain:{}\n", intGainChange );
									if ( maxsample * pow( 2.0, static_cast<double>(intGainChange) / 4.0 ) > 32767.0 ) { RST( "WARNING: some clipping may occur with this gain change!\n" ); }
									RST( "Max PCM sample at current gain:{:.3f}\n", maxsample );
									RST( "Max mp3 global gain field:{}\n", maxgain );
									RST( "Min mp3 global gain field:{}\n", mingain );
								}
							}
							else if ( opts->applyType == ByTrack ) {
								first = !false; /* don't keep track of Album gain */
								if ( inf ) fclose( inf );
								inf = NULL;
								goAhead = !false;

								if ( intGainChange == 0 ) {
									RST( "No changes to {} are necessary\n", curfilename );
									if ( !skipTag && tagInfo[fileIdx].dirty ) {
										RST( "...but tag needs update: Writing tag information for {}\n", curfilename );
										WriteMP3GainTag( curfilename, aacInfo[fileIdx], tagInfo + fileIdx, fileTags + fileIdx, opts->keepOrigDT );
									}
								}
								else {
									if ( opts->autoClip ) {
										int intMaxNoClipGain = static_cast<int>(floor( 4.0 * log10( 32767.0 / maxsample ) / log10( 2.0 ) ));
										if ( intGainChange > intMaxNoClipGain ) {
											RST( "Applying auto-clipped mp3 gain change of {} to {}\n(Original suggested gain was {})\n", intMaxNoClipGain, curfilename, intGainChange );
											intGainChange = intMaxNoClipGain;
										}
									}
									else if ( !opts->ignoreClipWarn ) {
										if ( maxsample * pow( 2.0, static_cast<double>(intGainChange) / 4.0 ) > 32767.0 ) {
											if ( queryUserForClipping( curfilename, intGainChange ) ) { RST( "Applying mp3 gain change of {} to {}...\n", intGainChange, curfilename ); }
											else { goAhead = 0; }
										}
									}
									if ( goAhead ) {
										RST( "Applying mp3 gain change of [{}] to {}\n", intGainChange, curfilename );
										if ( skipTag ) { changeGain( curfilename, aacH, intGainChange, intGainChange, rst, ireport, findFrame ); }
										else { changeGainAndTag( curfilename, aacH, intGainChange, intGainChange, tagInfo + fileIdx, fileTags + fileIdx, rst, ireport, findFrame ); }
									}
									else if ( !skipTag && tagInfo[fileIdx].dirty ) {
										RST( "Writing tag information for {}\n", curfilename );
										WriteMP3GainTag( curfilename, aacH, tagInfo + fileIdx, fileTags + fileIdx, opts->keepOrigDT );
									}
								}
							}
						}
					}
				}

				if ( !aacH ) ExitMP3( &mp );
				fflush( stderr );

				if ( inf ) fclose( inf );
				inf = NULL;
			}
		}
	}

	if ( numFiles > 0 && opts->applyType != ByTrack && !opts->anaTrack ) {
		if ( albumRecalc & FULL_RECALC ) {
			if ( maxAmpOnly ) dBchange = 0;
			else dBchange = GetAlbumGain();
		}
		else {
			/* the following if-else is for the weird case where someone applies "Album" gain to
			   a single file, but the file doesn't actually have an Album field */
			dBchange = tagInfo[0].haveAlbumGain ? tagInfo[0].albumGain : tagInfo[0].trackGain;
		}

		if ( dBchange == GAIN_NOT_ENOUGH_SAMPLES ) {
			RST( "Not enough samples in mp3 files to do analysis\n" );

		}
		else {
			Float_t maxmaxsample;
			unsigned char maxmaxgain;
			unsigned char minmingain;
			maxmaxsample = 0;
			maxmaxgain = 0;
			minmingain = 255;
			for ( fileIdx = 0; fileIdx < totFiles; fileIdx++ ) {
				if ( fileok[fileIdx] ) {
					if ( tagInfo[fileIdx].trackPeak > maxmaxsample ) maxmaxsample = tagInfo[fileIdx].trackPeak;
					if ( tagInfo[fileIdx].maxGain > maxmaxgain ) maxmaxgain = tagInfo[fileIdx].maxGain;
					if ( tagInfo[fileIdx].minGain < minmingain ) minmingain = tagInfo[fileIdx].minGain;
				}
			}

			if ( !skipTag && ( numFiles > 1 || opts->applyType == ByAlbum ) ) {
				for ( fileIdx = 0; fileIdx < totFiles; fileIdx++ ) {
					curTag = tagInfo + fileIdx;
					if ( !maxAmpOnly ) {
						if ( /* if we don't already have a tagged track gain OR we have it, but it doesn't match */
							!curTag->haveAlbumGain || ( curTag->haveAlbumGain && fabs( dBchange - curTag->albumGain ) >= 0.01 ) ) {
							curTag->dirty = !false;
							curTag->haveAlbumGain = 1;
							curTag->albumGain = dBchange;
						}
					}

					if ( !curTag->haveAlbumMinMaxGain || /* if albumMinGain or albumMaxGain doesn't match tag */
						( curTag->haveAlbumMinMaxGain && ( curTag->albumMinGain != minmingain || curTag->albumMaxGain != maxmaxgain ) ) ) {
						curTag->dirty = !false;
						curTag->haveAlbumMinMaxGain = !false;
						curTag->albumMinGain = minmingain;
						curTag->albumMaxGain = maxmaxgain;
					}

					if ( !curTag->haveAlbumPeak || ( curTag->haveAlbumPeak && fabs( maxmaxsample - curTag->albumPeak ) >= 0.0001 ) ) {
						curTag->dirty = !false;
						curTag->haveAlbumPeak = !false;
						curTag->albumPeak = maxmaxsample;
					}
				}
			}

			/* the TAG version of the suggested Album Gain should ALWAYS be based on the 89dB standard.
			   So we don't modify the suggested gain change until this point */

			dBchange += opts->dbGainMod;

			dblGainChange = dBchange / ( 5.0 * log10( 2.0 ) );
			if ( fabs( dblGainChange ) - static_cast<double>((int)fabs( dblGainChange )) < 0.5 ) intGainChange = static_cast<int>(dblGainChange);
			else intGainChange = static_cast<int>(dblGainChange) + ( dblGainChange < 0 ? -1 : 1 );
			intGainChange += opts->mp3GainMod;


			if ( opts->dbFormat ) {
				RST( "\"Album\"\t{}\t{:.3f}\t{:.3f}\t{}\t{}\n", intGainChange, dBchange, maxmaxsample * 32768.0, maxmaxgain, minmingain );

			}

			if ( opts->applyType != ByAlbum ) {
				if ( !opts->dbFormat ) {
					RST( "Rec Album dB change: {:.3f}\n", dBchange );
					RST( "Rec Album mp3 gain : {}\n", intGainChange );
					for ( fileIdx = 0; fileIdx < totFiles; fileIdx++ ) { if ( fileok[fileIdx] ) if ( tagInfo[fileIdx].trackPeak * pow( 2.0, static_cast<double>(intGainChange) / 4.0 ) > 1.0 ) { RST( "WARNING: with this global gain change, some clipping may occur in file {}\n", _GETFNAME(fileIdx) ); } }
				}
			}
			else {
				/*MAA*/
				if ( opts->autoClip ) {
					/*MAA*/
					int intMaxNoClipGain = static_cast<int>(floor( -4.0 * log10( maxmaxsample ) / log10( 2.0 ) ));
					/*MAA*/
					if ( intGainChange > intMaxNoClipGain ) {
						/*MAA*/
						RST( "Applying auto-clipped mp3 gain change of {} to album\n(Original suggested gain was {})\n", intMaxNoClipGain, intGainChange );
						/*MAA*/
						intGainChange = intMaxNoClipGain;
						/*MAA*/
					}
					/*MAA*/
				}
				for ( fileIdx = 0; fileIdx < totFiles; fileIdx++ ) {
					if ( fileok[fileIdx] ) {
						goAhead = !false;
						if ( intGainChange == 0 ) {
							RST( "\nNo changes to {} are necessary\n", _GETFNAME(fileIdx) );
							if ( !skipTag && tagInfo[fileIdx].dirty ) {
								RST( "...but tag needs update: Writing tag information for {}\n", _GETFNAME(fileIdx) );
								WriteMP3GainTag( _GETFNAME( fileIdx ), aacInfo[fileIdx], tagInfo + fileIdx, fileTags + fileIdx, opts->keepOrigDT );
							}
						}
						else {
							if ( !opts->ignoreClipWarn ) { if ( tagInfo[fileIdx].trackPeak * pow( 2.0, static_cast<double>(intGainChange) / 4.0 ) > 1.0 ) goAhead = queryUserForClipping( _GETFNAME( fileIdx ), intGainChange ); }
							if ( goAhead ) {
								RST( "Applying mp3 gain change of {} to {}...\n", intGainChange, _GETFNAME(fileIdx) );

								auto ireport = lambdaToFunctionPointer
								(
									[&]( unsigned long percent, unsigned long bytes ) -> unsigned long {
										//std::cerr << "[report] idx:" << numFiles << ", percent: " << percent << std::endl << std::flush;
										if ( opts->onProgress ) opts->onProgress( fileIdx+1, percent, bytes );
										return 1;
									}
								);

								if ( skipTag ) { changeGain( _GETFNAME( fileIdx ), aacInfo[fileIdx], intGainChange, intGainChange, rst, ireport, makeFindFrame( _GETFNAME( fileIdx ), rst ) ); }
								else { changeGainAndTag( _GETFNAME( fileIdx ), aacInfo[fileIdx], intGainChange, intGainChange, tagInfo + fileIdx, fileTags + fileIdx, rst, ireport, makeFindFrame( _GETFNAME( fileIdx ), rst ) ); }
							}
							else if ( !skipTag && tagInfo[fileIdx].dirty ) {
								RST( "Writing tag information for {}\n", _GETFNAME(fileIdx) );
								WriteMP3GainTag( _GETFNAME( fileIdx ), aacInfo[fileIdx], tagInfo + fileIdx, fileTags + fileIdx, opts->keepOrigDT );
							}
						}
					}
				}
			}
		}
	}

	/* update file tags */
	if ( opts->applyType != ByTrack && opts->applyType != ByAlbum && !opts->directGain && !opts->directChId && !deleteTag && !skipTag && !checkTagOnly ) {
		/* if we made changes, we already updated the tags */
		for ( fileIdx = 0; fileIdx < totFiles; fileIdx++ ) {
			if ( fileok[fileIdx] ) {
				if ( tagInfo[fileIdx].dirty ) {
					WriteMP3GainTag( _GETFNAME( fileIdx ), aacInfo[fileIdx], tagInfo + fileIdx, fileTags + fileIdx, opts->keepOrigDT );
				}
			}
		}
	}

	free( tagInfo );
	free( fileok );
	for ( fileIdx = 0; fileIdx < totFiles; fileIdx++ ) {
		if ( fileTags[fileIdx].apeTag ) {
			if ( fileTags[fileIdx].apeTag->otherFields ) { free( fileTags[fileIdx].apeTag->otherFields ); }
			free( fileTags[fileIdx].apeTag );
		}
		if ( fileTags[fileIdx].lyrics3tag ) { free( fileTags[fileIdx].lyrics3tag ); }
		if ( fileTags[fileIdx].id31tag ) { free( fileTags[fileIdx].id31tag ); }
		if ( aacInfo[fileIdx] ) aac_close( aacInfo[fileIdx] ); //close any open aac files
	}
	free( fileTags );
	free( aacInfo );

	RST( "gSuccess: {}\n", gSuccess );

	return rst.str();
}
std::string mainGain( std::vector<std::filesystem::path> &paths, const gainOpts &inOpts ) {
	std::vector<std::string> strings;
	strings.reserve( paths.size() );

	std::transform( paths.begin(), paths.end(), std::back_inserter( strings ),
		[]( const std::filesystem::path &path ) { return path.string(); } );

	return mainGain( strings, inOpts );
}

std::string mainGain( std::vector<std::string> &paths ) {
	return mainGain( paths, { .anaTrack = 1 } );
}


}
