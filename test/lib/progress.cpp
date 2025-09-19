#include "doctest.h"
#include <string>
#include "utils.hpp"
#include "rz/idx.h"

using namespace progress;

TEST_CASE( "progress: by_percent" ) {
	auto pt = 0;

	progress::TML::setIdeMode( true );

	BarOpts op;
	op.txtL = "test";
	op.width = 30;
	op.color = Color::Green;
	op.showTimeUsed = true;
	op.showTimeRemain = true;
	op.showPercent = true;
	op.vMax = 100;
	op.vMin = 0;
	op.styles = { Style::Bold };

	auto bar = Bar( op );

	std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
	while ( true ) {
		bar.setProgress( pt );
		pt += 5;

		if ( bar.isDone() ) break;

		std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
	}

	lg::info( "done" );
}

TEST_CASE( "progress: base1" ) {
	lg::info( "TEST START!" );

	auto p = rz::code::prog::makeBar( "測試" );

	p->setOpts( []( BarOpts &opt ) {

		opt.width = 50;
		opt.remainder = ".";
		opt.txtL = "Start...";
		opt.color = Color::Green;

	} );

	auto job = [&p]() {
		while ( true ) {
			auto ticks = p->current();
			if ( ticks > 20 && ticks < 50 ) p->setPrefix( "Delaying the inevitable" );
			else if ( ticks > 50 && ticks < 80 ) p->setPrefix( "Crying quietly" );
			else if ( ticks > 80 && ticks < 98 ) p->setPrefix( "Almost there" );
			else if ( ticks >= 98 ) p->setPrefix( "Done" );
			p->tick();
			if ( p->isDone() ) break;
			std::this_thread::sleep_for( std::chrono::milliseconds( 30 ) );
			//fflush(stdout);
		}
	};
	std::thread thread( job );
	thread.join();
}