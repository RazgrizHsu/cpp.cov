#pragma once

#include <array>
#include <string_view>
#include <stdexcept>
#include <vector>


namespace rz::io::type {

#define X( name ) name,
#define FILE_TYPES \
    X(unknown) X(jpeg) X(png) X(gif) X(bmp) X(tiff) X(webp) X(pdf) X(zip) X(rar) \
    X(seven_z) X(tar) X(elf) X(exe) X(doc) X(docx) X(xls) X(xlsx) X(ppt) X(pptx) \
    X(odf) X(rtf) X(mp3) X(wav) X(flac) X(m4a) X(ogg) X(avi) X(mp4) X(mkv) \
    X(webm) X(mov) X(flv) X(html) X(xml) X(sql) X(text)

enum class FileType { FILE_TYPES };

#undef X

// 存儲枚舉名稱的 string_view
constexpr auto fileTypeStrings = []()
{
#define X( name ) +1
	constexpr size_t size =
	FILE_TYPES
	;
#undef X

	std::array<std::string_view, size> arr{};
	size_t i = 0;

#define X( name ) arr[i++] = #name;
	FILE_TYPES
#undef X

	return arr;
}();

#undef FILE_TYPES

constexpr std::string_view toStr( FileType type )
{
	auto index = static_cast<size_t>(type);
	if ( index < fileTypeStrings.size() ) return fileTypeStrings[index];
	throw std::runtime_error( "Unknown FileType" );
}

constexpr FileType toFileType( std::string_view typeStr )
{
	auto it = std::ranges::find( fileTypeStrings.begin(), fileTypeStrings.end(), typeStr );
	if ( it != fileTypeStrings.end() ) return static_cast<FileType>(std::distance( fileTypeStrings.begin(), it ));
	throw std::runtime_error( "Invalid FileType string" );
}

FileType detect_file_type( const unsigned char *buffer, size_t size );
inline FileType detect_file_type( std::vector<unsigned char> vec ) {

	auto size = vec.size();
	return detect_file_type( vec.data(), size );
}


}
