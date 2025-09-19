#pragma once

#include <set>
#include <rz/sys/cmdr.h>



namespace rz::mda {

namespace fs = std::filesystem;

inline optional<vector<string>> cmdConv( const string &cmd, const string &str = "conv:" ) {

	regex rxDui( R"(Duration: ([0-9\.\:]+))" ); //Duration: 00:06:48.12
	regex rxDur( R"(time=([0-9\.\:]+) )" );
	smatch mth;

	set<string> strs;
	strs.insert( cmd );


	auto bar = rz::code::prog::makeBar( rz::fmt( " > {}", str::limit( str, 18 ) ) );

	dt::IDU dutot;

	auto actOut = [&]( bool isSTD, const std::string &line ) {

		//lg::info( "--{}-- {}", isSTD ? "O" : "E", line );

		if ( regex_search( line, mth, rxDui ) && mth[1].matched ) {

			auto du = rz::dt::IDU( mth[1] );
			//lg::info( "[] duration => old[{}] new[{}] ({})", dutot.secs(), du.secs(), line );
			dutot += du;


			bar->setPostfix( rz::fmt( "{}", dutot.strSecs() ) );
		}

		if ( regex_search( line, mth, rxDur ) && mth[1].matched ) {

			//string mt = mth[1];
			//lg::info( "dr: {}", mt );
			auto dunow = rz::dt::IDU( mth[1] );
			auto cn = dunow.secs();
			auto ct = dutot.secs();

			auto p = ( cn / ct ) * 100;
			bar->setProgress( p );

			bar->setPostfix( rz::fmt( "{} ({}/{})", str::limit( str, 30 ), dunow.strSecs(), dutot.strSecs() ) );
			//lg::info( "{} / {} = {:.2f}", cn, ct, p );
		}

		auto lines = str::splitNewLines( line );
		strs.insert( line );
	};

	try {
		int rst = rz::exec( cmd, actOut );
		if ( rst != 0 ) {
			lg::error( "[cmdConv] ret[ {} ]", rst );
			//for ( auto &ln: strs ) lg::warn( "--> {}", ln );
			strs.insert( rz::fmt( "ret:{}", rst ) );

			return vector( strs.begin(), strs.end() );
		}
		return std::nullopt;
	}
	catch ( std::exception &ex ) {
		auto msg = rz::fmt( "cmdConv異常, cmd[{}], msg[{}]", cmd, ex.what() );
		lg::warn( msg );
		strs.insert( msg );

		return vector( strs.begin(), strs.end() );
	}
}


}
