#include "rz/idx.h"
using namespace progress;


struct makeBarOpt {

	size_t max = 100;
	bool isBars = false;
};

inline shared_ptr<Bar> makeBar( const string &str = "?👀?", makeBarOpt opts = makeBarOpt{} ) {
	BarOpts op;
	op.txtL = str;
	op.width = 25;
	op.color = Color::Yellow;
	op.showTimeUsed = true;
	op.showTimeRemain = true;
	op.showPercent = true;
	op.vMax = opts.max;
	op.styles = { Style::Bold };

	if ( opts.isBars ) return Bars::shared()->make( op );
	return make_shared<Bar>( op );
}

void test_Bars_groups() {
	lg::info( "Dynamic Bars Test START! v3" );

	auto bars = Bars::shared();

	BarOpts bopt;
	bopt.width = 40;
	bopt.showPercent = true;
	bopt.styles = { Style::Bold };


	std::vector colors = {
		Color::Blue, Color::Green, Color::Yellow,
		Color::Red, Color::Magenta, Color::Cyan
	};

	TaskPool tsks( 3 );

	std::vector<std::shared_ptr<Bar>> spBars;

	for ( int i = 0; i < 6; ++i ) {

		tsks.enqueue( [&, i]() {
			BarOpts opts = bopt;
			opts.color = colors[i % colors.size()];
			opts.txtL = "Task " + std::to_string( i + 1 ) + ": ";

			lg::info( "執行: {}", opts.txtL );

			size_t ms = 10 + i * 6;
			auto bar = makeBar( opts.txtL, { .isBars = true } );
			for ( size_t j = 0; j <= 100; ++j ) {
				bar->setProgress( j );
				std::this_thread::sleep_for( std::chrono::milliseconds( ms ) );
			}
			lg::info( "done: {}", opts.txtL );

		} );

	}


	tsks.runWaitAll();

	std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
	lg::info( "All tasks completed. Wait 5s before cleaning..." );

	lg::info( "Dynamic Bars Test DONE!" );
}

void test_Singles( int cnt = 3 ) {
	auto pt = 0;

	lg::info( "start single..." );

	TaskPool tsks( 3 );

	std::vector<std::shared_ptr<Bar>> spBars;

	for ( int i = 0; i < cnt; ++i ) {
		tsks.enqueue( [&, i]() {

			if ( i > 0 ) {
				// 計算前一個進度條完成一半所需的大約時間
				size_t prev_bar_half_time = ( 100 + ( i - 1 ) * 20 ) * 50; // 前一個進度條的時間 * 50次迭代
				std::this_thread::sleep_for( std::chrono::milliseconds( prev_bar_half_time / 2 ) );
			}
			lg::info( "start: {}", i );

			BarOpts op;
			op.txtL = rz::fmt( "test:{}", i );
			op.width = 30;
			op.color = Color::Green;
			op.showTimeUsed = true;
			op.showTimeRemain = true;
			op.showPercent = true;
			op.vMax = 100;
			op.vMin = 0;
			op.styles = { Style::Bold };

			auto bar = Bar( op );

			size_t ms = 100 + ( i * 20 );

			while ( true ) {
				bar.setProgress( pt );
				pt += 1;

				if ( bar.isDone() ) break;

				std::this_thread::sleep_for( std::chrono::milliseconds( ms ) );
			}

			lg::info( "done: {}", i );
		} );
	}

	tsks.runWaitAll();
}



int main() {
	rz::code::prog::setCompatibleLogger();

	test_Singles( 3 );

	lg::info( "Done" );

	test_Bars_groups();

	return 0;
}
