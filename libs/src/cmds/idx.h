#pragma once

#include "./cli.h"
#include "./clihelp.h"
#include "./cov/idx.h"

namespace cmds { namespace {

static void OnCancel( int signal ) {
	lg::info( "🌼🌼🌼🌼🌼🌼 Cancelled 🌼🌼🌼🌼🌼🌼" );
	lg::shutdown();
	exit( 0 );
}

}}


namespace cmds {

using IFnCmd = std::function<void()>;

struct opts {
	bool logToFile = false;
};

inline void run( const fs::path &path, IFnCmd fn, opts opt = {} ) {

	rz::setUTF8();

	signal( SIGINT, OnCancel );

	if( !opt.logToFile ) {
		lg::default_logger()->set_level( lg::level::info );
	}
	else {

		auto now = rz::dt::IDT();
		auto logname = rz::fmt( "-cov-{}.log", now.str( "YYYYMMDDHHmmss" ) );
		auto logpath = path / logname;
		auto lgcons = make_shared<lg::sinks::stdout_color_sink_mt>();
		lgcons->set_level( lg::level::info );

		auto lgfile = make_shared<lg::sinks::basic_file_sink_mt>( logpath );
		lgfile->set_level( lg::level::trace );
		auto logger = make_shared<lg::logger>( lg::logger( "cov", { lgcons, lgfile } ) );

		lg::register_logger( logger );
		lg::set_default_logger( logger );

		lg::flush_every( std::chrono::seconds( 5 ) );

	}

	try { fn(); }
	catch ( std::exception &ex ) { lg::error( "[cov] 發生異常! {}", ex.what() ); }

	lg::shutdown();
}


}
