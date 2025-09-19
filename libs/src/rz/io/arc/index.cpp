#include <iostream>
#include <string>
#include <filesystem>

namespace rz::io::arc {

// void extract( const std::vector<unsigned char> &data ) {
//
// 	IArchive arc;
// 	// if( arc.open_memory( data ) != ARCHIVE_OK ) throw std::runtime_error( "Unable open as archive" );
//
// 	// auto queue = arc.getAll();
// 	// for( auto e : queue )
// 	// {
// 	// 	auto entry = e.get();
// 	//
// 	// 	std::cout << std::string( 40, '-' ) << std::endl;
// 	// 	std::cout << entry->path.string() << std::endl;
// 	//
// 	// 	auto vec = entry->read();
// 	// 	if ( !vec ) {
// 	// 		std::cerr << "Error reading data: " << arc.errStr() << std::endl;
// 	// 		return;
// 	// 	}
// 	//
// 	// 	auto type = detect_file_type( *vec );
// 	//
// 	// 	std::cout << "File: " << entry->path << "(" << toStr( type ) << ")" << std::endl;
// 	// 	std::cout << "Size: " << entry->size << std::endl;
// 	// 	std::cout << "mtime: " << entry->mtime << std::endl;
// 	// 	char time_str[100];
// 	// 	strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&entry->mtime));
// 	// 	std::cout << "mtime: " << time_str << std::endl;
// 	// }
// }


}
