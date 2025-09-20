#pragma once

#include <set>
#include <regex>
#include <rz/base/idx.h>
#include <rz/coll.hpp>

namespace rz::io {
namespace fs = std::filesystem;

class IDir;


using IFnOnDir = std::function<bool( const IDir & )>;
using IFnOnPath = std::function<bool( const fs::path & )>;

struct IFindRst {

	//決定是否要往下一層
	bool toDeep = true;
	bool include = true;
};
using IFnOnFindDir = std::function<IFindRst( const IDir & )>;


struct FindOpt {

	IFnOnDir onDir = nullptr;
};


//========================================================================
// IDir
//========================================================================
struct IDirInfo {
	unsigned int files = 0;
	unsigned int dirs = 0;
	unsigned int deep = 0;

	IDirInfo &operator+=( const IDirInfo &o ) {
		files += o.files;
		dirs += o.dirs;
		deep = std::max( deep, o.deep + 1 );
		return *this;
	}

	std::string str() const {
		std::stringstream ss;
		ss << "IDirInfo{deep:" << deep << "|dirs:" << dirs << "|files:" << files << "}";
		return ss.str();
	}
};

inline std::ostream &operator<<( std::ostream &os, const IDirInfo &obj ) { return os << obj.str(); }

struct Opts {

	shared_ptr<IDir> parent = nullptr;
	IFnOnDir onCheckDir = nullptr;
	IFnOnPath onCountFile = nullptr;
};

class IDir : public std::enable_shared_from_this<IDir> {

	IDir( const fs::path &path, shared_ptr<IDir> parent = nullptr ) {
		this->path = path;
		this->parent = parent;
	}

public:
	IDirInfo infos;
	weak_ptr<IDir> parent;
	fs::path path;
	deque<fs::path> files;
	deque<shared_ptr<IDir>> dirs;

	IDir( IDir && ) noexcept = default; // 移動構造函式
	IDir &operator=( const IDir &other ) = delete;
	IDir( const IDir &other ) {
		infos = other.infos;
		parent = other.parent;
		path = other.path;
		files = other.files;
		dirs = other.dirs;
	}

	IDir &operator=( const shared_ptr<IDir> &other ) {
		if ( this != other.get() ) {
			infos = other->infos;
			parent = other->parent;
			path = other->path;
			files = other->files;
			dirs = other->dirs;
		}
		return *this;
	}

	virtual ~IDir() = default; // 虛析構函式

	string str() const;

public:
	template<typename T> static
	shared_ptr<IDir> make( T &&path, Opts opt = {} ) {

		if ( !fs::is_directory( path ) ) return nullptr;

		auto dir = shared_ptr<IDir>( new IDir( std::forward<T>( path ), opt.parent ) );
		IDirInfo cs;

		vector<fs::path> paths;

		for ( const auto &entry: fs::directory_iterator( path ) ) {
			auto path = entry.path();

			if ( entry.is_directory() ) { paths.push_back( path ); }
			else {
				if ( path.filename() == ".DS_Store" ) continue;
				if ( opt.onCountFile && !opt.onCountFile( path ) ) continue;

				dir->files.emplace_back( path );
				cs.files++;
			}
		}

		//如果有並且為ture, 表示略過當前資料夾
		if ( opt.onCheckDir && !opt.onCheckDir( *dir ) ) return nullptr;


		for ( const auto &path: paths ) {
			auto cdir = make( path, { .parent = dir, .onCheckDir = opt.onCheckDir, .onCountFile = opt.onCountFile } );
			if ( cdir ) {
				dir->dirs.emplace_back( cdir );
				cs.dirs++;
				cs += cdir->infos;
			}
		}

		dir->infos = cs;
		if ( opt.parent ) { opt.parent->updateParentCounts( cs ); }

		return dir;
	}

private:
	void updateParentCounts( const IDirInfo &childCounts ) const {

		auto parent = this->parent.lock();
		if ( parent ) {
			parent->infos += childCounts;
			parent->updateParentCounts( childCounts );
		}
	}

public:

	vector<fs::path> allFiles() const {
		vector<fs::path> paths;

		this->allFiles( paths );
		for( auto &d : this->dirs ) d->allFiles( paths );

		return paths;
	}
	void allFiles( vector<fs::path> &paths ) const {
		for( auto &p : this->files ) paths.push_back( p );
	}

	vector<fs::path> findF( const string &key, FindOpt opt = {} ) const {
		auto cmp = makeCmp( key, opt );
		return findF( cmp, opt );
	}

	vector<fs::path> findD( const string &key, FindOpt opt = {} ) const {
		auto cmp = makeCmp( key, opt );
		return findD( cmp, opt );
	}

	vector<fs::path> find( vector<string> names, FindOpt opt = {} ) const {
		vector<fs::path> paths;

		for ( auto &name: names ) {

			auto files = findF( name, opt );
			auto dirs = findD( name, opt );

			paths += files;
			paths += dirs;
		}

		return paths;
	}


#define S const string
#define PF vector<fs::path>
#define SF , FindOpt opt = {}
#define SA , opt
#define C const
	PF find( S &f1 SF ) C { return find( vector{ f1 } SA ); }
	PF find( S &f1, S &f2 SF ) C { return find( { f1, f2 } SA ); }
	PF find( S &f1, S &f2, S &f3 SF ) C { return find( { f1, f2, f3 } SA ); }
	PF find( S &f1, S &f2, S &f3, S &f4 SF ) C { return find( { f1, f2, f3, f4 } SA ); }
	PF find( S &f1, S &f2, S &f3, S &f4, S &f5 SF ) C { return find( { f1, f2, f3, f4, f5 } SA ); }
	PF find( S &f1, S &f2, S &f3, S &f4, S &f5, S &f6 SF ) C { return find( { f1, f2, f3, f4, f5, f6 } SA ); }
#undef C
#undef SA
#undef SF
#undef PF
#undef S

	vector<fs::path> find( const regex &rgx ) const {
		vector<fs::path> vecs;

		IFnOnPath finder = [&]( const fs::path &path ) {
			return std::regex_match( path.filename().string(), rgx );
		};

		findF( finder, vecs );
		findD( finder, vecs );

		return vecs;
	}

	vector<shared_ptr<IDir>> findDirs( IFnOnFindDir onDir ) const {
		vector<shared_ptr<IDir>> vecs;
		findDirs( onDir, vecs );
		return vecs;
	}

private:
	void findDirs( IFnOnFindDir &onDir, vector<shared_ptr<IDir>> &rets ) const {

		auto rst = onDir( *this );
		if ( rst.include ) rets.push_back( make_shared<IDir>( *this ) );

		if ( rst.toDeep ) {
			for ( const auto &dir: dirs ) dir->findDirs( onDir, rets );
		}
	}
	vector<fs::path> findF( IFnOnPath &cmper, FindOpt opt = {} ) const {
		vector<fs::path> vec;
		findF( cmper, vec, opt );
		return vec;
	}

	vector<fs::path> findD( IFnOnPath &cmper, FindOpt opt = {} ) const {
		vector<fs::path> vec;
		findD( cmper, vec, opt );
		return vec;
	}
	void findF( IFnOnPath &cmper, vector<fs::path> &paths, FindOpt opt = {} ) const {

		if ( opt.onDir && !opt.onDir( *this ) ) return; //如果check onDir回傳false表示略過

		for ( const auto &file: files ) {
			if ( cmper( file ) ) paths.emplace_back( file );
		}

		for ( const auto &dir: dirs ) {
			auto rst = dir->findF( cmper, opt );
			paths += rst;
		}
	}
	void findD( IFnOnPath &cmper, vector<fs::path> &paths, FindOpt opt = {} ) const {

		if ( opt.onDir && !opt.onDir( *this ) ) return; //如果check onDir回傳false表示略過

		for ( const auto &dir: dirs ) {
			if ( cmper( dir->path ) ) paths.emplace_back( dir->path );
			dir->findD( cmper, paths, opt );
		}
	}



	static IFnOnPath makeCmp( const string &key, FindOpt opt ) {

		if ( rz::str::contains( key, "*." ) ) {
			return [&]( const fs::path &path ) {
				auto target = key;
				str::replaceAll( target, "*", "" );
				auto name = path.filename().string();
				return name.ends_with( target );
			};
		}

		return [&]( const fs::path &path ) {
			auto name = path.filename().string();
			auto match =
					( name == key )                         //完全相等
					|| ( name.find( key ) != string::npos ) //任意位置
					|| ( rz::cmp::ignoreCase( name, key ) ) //略過大小寫
				;
			return match;
		};
	}

public:
	void del() const { fs::remove_all( path ); }

	fs::path rename( const string &newName, bool replace = false ) {

		auto pnew = this->path.parent_path() / newName;

		auto exist = fs::exists( pnew );
		if ( replace ) {
			if ( exist ) fs::remove_all( pnew );
		}
		else {
			if ( exist ) throw runtime_error( "目標路徑已存在: " + pnew.string() );
		}

		fs::rename( this->path, pnew );

		return pnew;
	}

};

inline std::ostream &operator<<( std::ostream &os, const IDir &obj ) { return os << obj.str(); }


static void removeRarPartsAll( std::vector<fs::path> &files ) {
	std::regex rgxRar( "part\\d+\\.rar$" );

	files.erase(
		std::remove_if( files.begin(), files.end(),
			[&]( const fs::path &file ) {
				std::string filename = file.filename().string();
				return std::regex_search( filename, rgxRar );
			} ),
		files.end()
	);
}

static void removeRarParts( std::vector<fs::path> &files ) {
	std::regex rgxRar( "part\\d+\\.rar$" );
	std::regex rgxRa1( "part1\\.rar$" );

	files.erase(
		std::remove_if( files.begin(), files.end(),
			[&]( const fs::path &file ) {
				std::string filename = file.filename().string();
				return std::regex_search( filename, rgxRar ) && !std::regex_search( filename, rgxRa1 );
			} ),
		files.end()
	);
}


}
