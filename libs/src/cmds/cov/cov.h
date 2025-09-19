#pragma once

#include "cmds/cov/defs.h"
namespace cmds::cov {

struct Opts {
	bool ignoreVideo = false;
	bool resize = true;
	bool forceIncr = false;

	shared_ptr<CovCfg> cc;
};


extern Opts ops;

void startBy( const fs::path &path, bool logFile = true );


void doVideos( const rz::io::IDir *path );


}
