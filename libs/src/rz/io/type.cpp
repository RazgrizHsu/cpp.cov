#include "./type.h"

#include <memory>
#include <cstring>
#include <vector>
#include <array>
#include <string_view>
#include <algorithm>
#include <stdexcept>


template<typename T>
int cmp(const T* ptr1, const char* ptr2, size_t num) { return std::memcmp(ptr1, ptr2, num); }

namespace rz::io::type {

FileType detect_file_type( const unsigned char *buffer, size_t size )
{
	if ( size < 4 ) return FileType::unknown;

	// Image formats
	if ( buffer[0] == 0xFF && buffer[1] == 0xD8 ) return FileType::jpeg;
	if ( size >= 8 && cmp( buffer, "\x89PNG\r\n\x1A\n", 8 ) == 0 ) return FileType::png;
	if ( size >= 6 && ( cmp( buffer, "GIF87a", 6 ) == 0 || cmp( buffer, "GIF89a", 6 ) == 0 ) ) return FileType::gif;
	if ( buffer[0] == 'B' && buffer[1] == 'M' ) return FileType::bmp;
	if ( cmp( buffer, "II*\0", 4 ) == 0 || cmp( buffer, "MM\0*", 4 ) == 0 ) return FileType::tiff;
	if ( size >= 12 && cmp( buffer, "RIFF", 4 ) == 0 && cmp( buffer + 8, "WEBP", 4 ) == 0 ) return FileType::webp;

	// Document formats
	if ( size >= 5 && cmp( buffer, "%PDF-", 5 ) == 0 ) return FileType::pdf;
	if ( cmp( buffer, "\xD0\xCF\x11\xE0", 4 ) == 0 ) return FileType::doc; // Also XLS, PPT
	if ( cmp( buffer, "PK\x03\x04", 4 ) == 0 ) return FileType::zip; // DOCX, XLSX, PPTX, ODF, etc.
	if ( size >= 6 && cmp( buffer, "Rar!\x1A\x07", 6 ) == 0 ) return FileType::rar;
	if ( size >= 6 && cmp( buffer, "7z\xBC\xAF\x27\x1C", 6 ) == 0 ) return FileType::seven_z;
	if ( size >= 5 && ( cmp( buffer, "ustar", 5 ) == 0 || cmp( buffer, "ustar\0", 6 ) == 0 ) ) return FileType::tar;
	if ( size >= 5 && cmp( buffer, "{\rtf", 5 ) == 0 ) return FileType::rtf;

	// Executable formats
	if ( cmp( buffer, "\x7F\x45\x4C\x46", 4 ) == 0 ) return FileType::elf;
	if ( buffer[0] == 'M' && buffer[1] == 'Z' ) return FileType::exe;

	// Audio formats
	if ( cmp( buffer, "ID3", 3 ) == 0 ) return FileType::mp3;
	if ( size >= 12 && cmp( buffer, "RIFF", 4 ) == 0 && cmp( buffer + 8, "WAVE", 4 ) == 0 ) return FileType::wav;
	if ( cmp( buffer, "fLaC", 4 ) == 0 ) return FileType::flac;
	if ( cmp( buffer, "OggS", 4 ) == 0 ) return FileType::ogg;

	// Video formats
	if ( size >= 12 && cmp( buffer, "RIFF", 4 ) == 0 && cmp( buffer + 8, "AVI ", 4 ) == 0 ) return FileType::avi;
	if ( size >= 12 && cmp( buffer, "\x00\x00\x00", 3 ) == 0 &&
		( buffer[3] == 0x18 || buffer[3] == 0x20 ) && cmp( buffer + 4, "ftyp", 4 ) == 0 )
	{
		if ( cmp( buffer + 8, "mp42", 4 ) == 0 ||
			cmp( buffer + 8, "isom", 4 ) == 0 ||
			cmp( buffer + 8, "mp41", 4 ) == 0 )
		{
			return FileType::mp4;
		}
		if ( cmp( buffer + 8, "M4A ", 4 ) == 0 )
		{
			return FileType::m4a;
		}
	}
	if ( cmp( buffer, "\x1A\x45\xDF\xA3", 4 ) == 0 ) return FileType::mkv;
	if ( cmp( buffer, "\x1A\x45\xDF\xA3", 4 ) == 0 ) return FileType::webm;
	if ( size >= 8 && cmp( buffer, "ftypqt  ", 8 ) == 0 ) return FileType::mov;
	if ( cmp( buffer, "FLV", 3 ) == 0 ) return FileType::flv;

	return FileType::unknown;
}

}
