#pragma once

#include <regex>
#include <rz/base/str.h>
#include <rz/code/concept.h>
#include <archive.h>
#include <archive_entry.h>
#include <spdlog/idx.h>


#include <iconv.h>
#include <errno.h>
#include <cstring>

// 常見的日文編碼列表
const std::vector<std::string> JAPANESE_ENCODINGS = {
	"UTF-8", "SHIFT-JIS", "EUC-JP", "ISO-2022-JP"
};

const int BLOCK_SIZE = 20480;

inline std::string detect_and_convert( const std::string &input ) {

	for ( const auto &encoding: JAPANESE_ENCODINGS ) {
		iconv_t cd = iconv_open( "UTF-8", encoding.c_str() );
		if ( cd == (iconv_t)-1 ) {
			lg::debug( "iconv_open failed for {}", encoding );
			continue;
		}

		const char *in_buf = input.c_str();
		size_t in_left = input.length();
		std::vector<char> out_buf( in_left * 4 );
		char *out_ptr = out_buf.data();
		size_t out_left = out_buf.size();

		size_t result = iconv( cd, const_cast<char **>(&in_buf), &in_left, &out_ptr, &out_left );
		iconv_close( cd );

		if ( result != (size_t)-1 ) {
			std::string converted( out_buf.data(), out_buf.size() - out_left );
			//lg::debug( "Detected encoding: {}", encoding );
			return converted;
		}
	}

	lg::debug( "Unable to detect encoding: {}", input );

	return input;
}

namespace rz::io::arc {

namespace fs = std::filesystem;

class IArcEntryInfo;
class IArcEntry;
class IArchive;

using IFn = std::function<void()>;

using IFnOnExtr = std::function<void( IArchive & )>;
using IFnOnExtrItem = std::function<void( IArcEntry * )>;
using IFnOnInfo = std::function<void( IArcEntryInfo & )>;

class IArcEntryInfo {

public:
	bool isDir;
	bool isFil;
	size_t size;
	fs::path path;
	time_t mtime;

	explicit IArcEntryInfo( archive_entry *entry ) {
		mode_t file_type = archive_entry_filetype( entry );
		isDir = file_type & AE_IFDIR;
		isFil = file_type & AE_IFREG;
		size = archive_entry_size( entry );

		auto name = archive_entry_pathname( entry ); //會死掉
		auto cname = detect_and_convert( name );

		path = cname;

		//lg::info( "[entry] path[{}] conv[{}]", path.filename().string(), cname );

		auto state = archive_entry_stat( entry );
		mtime = state->st_mtime;
	}
};


class IArcEntry : public IArcEntryInfo {

public:
	IArcEntry( ::archive *a, archive_entry *entry )
		: arc( a ), entry( entry ), IArcEntryInfo( entry )
	{
	}

	IArcEntry &operator=( const IArcEntryInfo &info ) {

		this->path = info.path;
		this->size = info.size;
		this->isDir = info.isDir;
		this->isFil = info.isFil;
		this->mtime = info.mtime;

		return *this;
	}

private:
	::archive *arc;
	::archive_entry *entry;

public:
	// file mode不能用, 會有異常
	std::shared_ptr<vector<unsigned char>> read() const {

		std::vector<unsigned char> buffer( size );
		auto ret = archive_read_data( arc, buffer.data(), size );
		if ( ret < 0 ) {
			auto err = archive_error_string( arc );
			auto msg = rz::fmt( "讀取[ {} ]資料失敗, {}", this->path.string(), err );
			throw std::runtime_error( msg );
		}

		return make_unique<vector<unsigned char>>( std::move( buffer ) );
	}

	void save( fs::path path ) const {
		archive_entry_set_pathname( entry, path.c_str() );
		auto ret = archive_read_extract( arc, entry, 0 );
		if ( ret != ARCHIVE_OK ) {
			auto err = archive_error_string( arc );
			auto msg = rz::fmt( "{}, Failed extract [{}] to [{}]", err, this->path.filename().string(), path.string() );
			throw std::runtime_error( msg );
		}
	}

	using IFnOnProgress = std::function<void( size_t, size_t )>;

	void saveTo( fs::path path, IFnOnProgress onProg ) const {

		std::vector<char> buffer( 10240 );

		size_t sizeTT = size;
		size_t sizePd = 0;

		if ( fs::exists( path ) ) {
			lg::warn( "輸出路徑已存在, 刪除: {}", path.string() );
			fs::remove( path );
		}

		std::ofstream outf( path, std::ios::binary );
		if ( !outf ) throw std::runtime_error( "Failed to create output file: " + path.string() );


		auto r = 0;
		const void *block;
		size_t block_size;
		int64_t offset;
		while ( ( r = archive_read_data_block( arc, &block, &block_size, &offset ) ) == ARCHIVE_OK ) {
			outf.write( (const char *)block, block_size );
			sizePd += block_size;
			if ( onProg ) onProg( sizePd, sizeTT );
		}

		if ( r != ARCHIVE_EOF ) {
			auto code = archive_errno( arc );
			const char *err = archive_error_string( arc );
			throw std::runtime_error( rz::fmt( "讀取檔案[{}]異常: code[{}] msg[{}]", path.filename().string(), code, err ) );
		}


		outf.close();

	}
};

class IArchive {
	::archive *arc;

	void open() {

		arc = archive_read_new();
		//archive_read_set_options( arc, "hdrcharset=SHIFT_JIS");
		//archive_read_set_options( arc, "hdrcharset=UTF-8");

		if ( !arc ) throw std::runtime_error( "Failed to create archive" );
		archive_read_support_filter_all( arc );
		archive_read_support_format_all( arc );
	}
	void close() const {
		if ( !arc ) return;
		archive_read_close( arc );
		archive_read_free( arc );
	}

public:
	IArchive() {
		this->infos = std::make_shared<std::vector<IArcEntryInfo>>();
		this->open();
	}
	~IArchive() { this->close(); }

	IArchive( const IArchive & ) = delete;
	IArchive &operator=( const IArchive & ) = delete;

	::archive *get() const { return arc; }
	const char *errStr() const { return archive_error_string( arc ); }

	shared_ptr<vector<IArcEntryInfo>> infos;

private:
	IFn onInit;

	void doInitialize() {
		archive_entry *entry;
		onInit();

		try {
			while ( archive_read_next_header( arc, &entry ) == ARCHIVE_OK ) {
				auto info = IArcEntryInfo( entry );
				//因為有的時候會有dir，有時候又沒有，乾脆全部dir都不要進去
				if ( !info.isDir && !info.path.filename().string().ends_with( ".DS_Store" ) ) infos->push_back( info );
			}

			this->close();
			this->open();

			onInit();
		}
		catch ( std::exception &ex ) {
			auto msg = archive_error_string( arc );
			throw std::runtime_error( rz::fmt( "[Archive] fail init! {}, {}", msg, ex.what() ) );
		}
	}

public:
	void onInfo( IFnOnInfo onInfo ) {

		auto is = *infos;
		for ( auto info: is ) onInfo( info );
	}

	void openFile( const fs::path &path, size_t block_size = BLOCK_SIZE ) {

		auto cfname = make_shared<const char *>( path.c_str() );
		auto size = block_size;
		onInit = [&]() {
			auto ret = archive_read_open_filename( arc, *cfname, size );
			if ( ret != ARCHIVE_OK ) {
				auto msg = archive_error_string( arc );
				throw std::runtime_error( rz::fmt( "[Archive] fail to open: {}, {}", path.string(), msg ) );
			}
		};

		doInitialize();
	}

	void openFile( const vector<fs::path> &paths, size_t block_size = BLOCK_SIZE ) {

		auto phs = std::make_shared<std::vector<std::string>>();
		phs->reserve( paths.size() );
		for ( const auto &path: paths ) phs->push_back( path.string() );

		auto cstrs = std::make_unique<const char *[]>( phs->size() + 1 );
		for ( size_t i = 0; i < phs->size(); ++i ) cstrs[i] = phs->at( i ).c_str();

		cstrs[phs->size()] = nullptr; // 確保以 null 結尾

		onInit = [&,block_size]() {
			auto ret = archive_read_open_filenames( arc, cstrs.get(), block_size );
			if ( ret != ARCHIVE_OK ) {

				auto names = str::join( *phs );
				auto msg = archive_error_string( arc );
				throw std::runtime_error( rz::fmt( "[Archive] fail to open: {}, {}", names, msg ) );
			}
		};

		doInitialize();
	}


	void openMemory( const std::vector<unsigned char> &vec ) {

		onInit = [&]() {
			auto ret = archive_read_open_memory( arc, vec.data(), vec.size() );
			if ( ret != ARCHIVE_OK ) {
				auto msg = archive_error_string( arc );
				throw std::runtime_error( rz::fmt( "[Archive] fail to open from memory: {}", msg ) );
			}
		};

		doInitialize();
	}



	void extr( IFnOnExtrItem onDo ) {

		archive_entry *entry;

		auto count = 0;
		auto code = ARCHIVE_OK;
		auto hasErr = false;
		while ( ( code = archive_read_next_header( arc, &entry ) ) == ARCHIVE_OK ) {

			count++;
			auto e = IArcEntry( arc, entry );
			if ( e.isDir || e.path.filename().string().ends_with( ".DS_Store" ) ) continue;

			auto name = rz::io::fil::stem( e.path.filename() );

			auto info = std::ranges::find_if( infos->begin(), infos->end(), [&e]( const IArcEntryInfo &i ) { return e.path == i.path; } );
			if ( info == infos->end() ) {

				auto vecs = vector<string>( infos->size() );
				ranges::transform( *infos, vecs.begin(),[](const IArcEntryInfo& info) { return info.path.filename().string(); });
				auto names = rz::str::join( vecs );

				auto msg = rz::fmt( "[Archive] 找不到EntryInfo, msg[{}] path[{}] 現有的[{}]", archive_error_string( arc ), name, names );
				throw std::runtime_error( msg );
			}
			e = *info;

			try { onDo( &e ); }
			catch ( const std::exception &ex ) {
				lg::warn( "[Archive] 解壓縮[{}]時發生異常, {}", e.path.filename().string(), ex.what() );
				hasErr = true;
			}
		}

		if ( count <= 0 && code != ARCHIVE_OK ) {
			auto msg = archive_error_string( arc );
			throw std::runtime_error( rz::fmt( "[Archive] code[{}] fail to read list: {}", code, msg ) );
		}

		if( hasErr ) throw runtime_error( "解壓縮過程有異常" );
	}



};



inline unique_ptr<vector<fs::path>> extract( const fs::path &path, IFnOnExtr onExtract ) {
	if ( !fs::exists( path ) ) throw std::runtime_error( "file not exist" );

	IArchive arc;
	auto paths = make_unique<vector<fs::path>>();

	//處理.part#.rar
	if ( path.extension() == ".rar" && path.stem().extension() == ".part1" ) {

		// 2. 查找同目錄下的其他 partN.rar 檔案
		fs::path dir = path.parent_path();
		std::string name = path.stem().stem().string();
		std::string namer = std::regex_replace( name, std::regex( R"([.^$|()\[\]{}*+?\\])" ), R"(\$&)" );
		std::regex rgx( namer + R"(\.part[0-9]+\.rar$)" );


		for ( const auto &e: fs::directory_iterator( dir ) ) {
			const auto &p = e.path();
			if ( !fs::is_regular_file( p ) ) continue;

			auto filename = p.filename().string();
			if ( std::regex_match( filename, rgx ) ) paths->push_back( p );
		}

		std::sort( paths->begin(), paths->end() );

		if ( paths->size() <= 1 ) {
			auto msg = rz::fmt( "找不到剩餘的rar檔案, {}", path );
			throw std::runtime_error( msg );
		}

		lg::info( "[rar] files[{}] {}", paths->size(), *paths );

		arc.openFile( *paths );
	}
	else {

		paths->push_back( path );

		arc.openFile( path );
	}

	onExtract( arc );

	return paths;
}

template<code::IVec T>
void extract( T &&data, IFnOnExtr onExtr ) {
	const auto &vec = rz::code::getRef( std::forward<T>( data ) );

	IArchive arc;
	arc.openMemory( vec );

	onExtr( arc );
}



}
