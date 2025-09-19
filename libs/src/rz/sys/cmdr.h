#pragma once

#include <string>
#include <functional>
#include <future>
#include <stdexcept>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <spdlog/idx.h>

namespace rz {

using IFnOnLine = std::function<void( bool isStdOut, const std::string &line )>;

inline const IFnOnLine onLineLogDefault = []( bool isStdOut, const std::string &s ) {
	isStdOut ? lg::info( "[cmdr] {}", s ) : lg::error( "[cmdr] {}", s );
};


class Cmdr {
public:
	Cmdr( bool debug = false ) : dbg( debug ) {}

	std::future<int> execAsync( const std::string &cmd, IFnOnLine onLine ) {
		return std::async( std::launch::async, [this, cmd, onLine ]() {
			return this->exec( cmd, onLine );
		} );
	}

private:
	bool dbg;

	void processOutput( int fd, const IFnOnLine &onOutput, bool isStdOut ) {
		char buf[4096];
		ssize_t count;
		std::string line;

		while ( ( count = read( fd, buf, sizeof( buf ) ) ) > 0 ) {
			for ( ssize_t i = 0; i < count; ++i ) {
				if ( buf[i] != '\n' && buf[i] != '\r' ) line += buf[i];
				else {
					if ( !line.empty() ) {
						onOutput( isStdOut, line );
						line.clear();
					}
				}
			}
		}

		if ( !line.empty() ) onOutput( isStdOut, line );
	}

	int exec( const std::string &cmd, const IFnOnLine &onOutput ) {
		int ppout[2], pperr[2];
		if ( pipe( ppout ) == -1 || pipe( pperr ) == -1 ) {
			throw std::runtime_error( "Failed to create pipes" );
		}

		pid_t pid = fork();
		if ( pid == -1 ) {
			close( ppout[0] );
			close( ppout[1] );
			close( pperr[0] );
			close( pperr[1] );
			throw std::runtime_error( "Fork failed" );
		}

		if ( pid == 0 ) { // Child process
			close( ppout[0] );
			close( pperr[0] );
			dup2( ppout[1], STDOUT_FILENO );
			dup2( pperr[1], STDERR_FILENO );
			close( ppout[1] );
			close( pperr[1] );

			execl( "/bin/sh", "sh", "-c", cmd.c_str(), nullptr );
			_exit( 1 );
		}

		// Parent process
		close( ppout[1] );
		close( pperr[1] );

		std::thread thOut( [&]() { processOutput( ppout[0], onOutput, true ); } );
		std::thread thErr( [&]() { processOutput( pperr[0], onOutput, false ); } );

		int status;
		waitpid( pid, &status, 0 );

		thOut.join();
		thErr.join();

		close( ppout[0] );
		close( pperr[0] );

		if ( WIFEXITED( status ) ) return WEXITSTATUS( status );
		if ( WIFSIGNALED( status ) ) return 128 + WTERMSIG( status );

		return -999; // Unknown error
	}
};


inline int exec( const std::string &cmd, IFnOnLine onOutput = onLineLogDefault ) {
	Cmdr cmder;
	try {
		auto future = cmder.execAsync( cmd, onOutput );
		return future.get();
	}
	catch ( std::exception &ex ) {
		lg::error( "[cmdr] Execution failed: {}", ex.what() );
		throw std::runtime_error( "[cmdr] Execution failed: " + std::string( ex.what() ) );
	}
}

}
