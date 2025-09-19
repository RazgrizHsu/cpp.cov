#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include <queue>
#include <functional>
#include <map>
#include <sys/ioctl.h>
#include <unistd.h>
#include "./help/termcolor.hpp"
#include "./unicode.hpp"

namespace progress {

struct TML {
	static inline std::string GoH = "\r";       // 將遊標移至行首
	static inline std::string CLn = "\r\033[K"; // 清除當前行
	static inline std::string MvUp = "\033[1A"; // 將遊標上移一行
	static inline std::string NewL = "\n";      // 換行
	static inline std::string C2E = "\033[K";   // 從遊標清除到行尾

	static inline std::string MvAbove = "\n\033[A\033[1L";

	static inline bool enable = false;

	// For IDE environments like CLion that don't support ANSI sequences
	static void setIdeMode( bool enabled ) {
		enable = enabled;
		if ( enabled ) {
			GoH = "\n";
			MvUp = "";
		}
		else {
			GoH = "\r";
			MvUp = "\033[1A";
		}
	}
};

enum class Color { None, Grey, Red, Green, Yellow, Blue, Magenta, Cyan, White };
enum class Style { None, Bold, Dark, Italic, Underline, Blink, Reverse, Concealed, Crossed };

struct BaseOpts {
	std::string txtL = "", txtR = "";
	bool showPercent = false, showTimeUsed = true, showTimeRemain = false;
	Color color = Color::None;
	std::vector<Style> styles;
	size_t vMax = 100, vMin = 0;
	bool fillSpace = false;
	std::ostream *ptrOuter = &std::cout;
};

struct BarOpts : BaseOpts {
	size_t width = 100;
	std::string start = "[", end = "]", fill = "=", lead = ">", remainder = "-";
};

class GuardOs {
public:
	explicit GuardOs( std::ostream &os ) : os_( os ) {}
	~GuardOs() { os_ << termcolor::reset; }

private:
	std::ostream &os_;
};



static std::string getTimeUsed( const std::chrono::nanoseconds &ns ) {
	auto h = std::chrono::duration_cast<std::chrono::hours>( ns );
	auto m = std::chrono::duration_cast<std::chrono::minutes>( ns - h );
	auto s = std::chrono::duration_cast<std::chrono::seconds>( ns - h - m );
	char buf[9];
#ifdef __APPLE__
	snprintf( buf, sizeof( buf ), "%02ld:%02ld:%02lld", h.count(), m.count(), s.count() );
#else
	snprintf(buf, sizeof(buf), "%02ld:%02ld:%02ld", h.count(), m.count(), s.count());
#endif
	return buf;
}

static std::string getTimeRemain( const std::chrono::nanoseconds &elapsed, size_t progress, size_t maxProgress ) {
	if ( progress == 0 ) return "--:--:--";
	const auto total = elapsed * maxProgress / progress;
	return getTimeUsed( total - elapsed );
}

static size_t displayWidth( const std::string &s ) {
	return ranges::count_if( s, []( char c ) { return ( c & 0xC0 ) != 0x80; } );
}

static std::pair<size_t, size_t> terminalSize() {
	winsize w;
	ioctl( STDOUT_FILENO, TIOCGWINSZ, &w );
	return { w.ws_row, w.ws_col };
}



class Printer : public std::enable_shared_from_this<Printer> {
public:
	static std::shared_ptr<Printer> shared() {
		static std::shared_ptr<Printer> instance( new Printer() );
		return instance;
	}

	void addJob( std::function<void()> job ) {
		std::lock_guard lock( _mtx );
		auto now = std::chrono::steady_clock::now();
		if ( now - timeLast >= std::chrono::milliseconds( 33 ) ) {
			quePr.push( std::move( job ) );
			timeLast = now;
			qcv.notify_one();
		}
	}

	void stop() {
		{
			std::lock_guard lock( _mtx );
			isStop = true;
		}
		qcv.notify_one();
		if ( threadPtr.joinable() ) threadPtr.join();
	}

	~Printer() { stop(); }
	Printer( const Printer & ) = delete;
	Printer &operator=( const Printer & ) = delete;
	Printer( Printer && ) = delete;
	Printer &operator=( Printer && ) = delete;

private:
	Printer() : timeLast( std::chrono::steady_clock::now() ) {
		threadPtr = std::thread( &Printer::printLoop, this );
	}

	void printLoop() {
		while ( true ) {
			std::function<void()> job;
			{
				std::unique_lock lock( _mtx );
				qcv.wait( lock, [this] { return isStop || !quePr.empty(); } );
				if ( isStop && quePr.empty() ) return;
				if ( !quePr.empty() ) {
					job = std::move( quePr.front() );
					quePr.pop();
				}
			}
			if ( job ) job();
		}
	}

	std::queue<std::function<void()>> quePr;
	std::mutex _mtx;
	std::condition_variable qcv;
	std::thread threadPtr;
	bool isStop = false;
	std::chrono::steady_clock::time_point timeLast;
};

class IBar {
public:
	virtual ~IBar() = default;
	virtual void setProgress( size_t v ) = 0;
	virtual void tick() = 0;
	virtual void printProgress() = 0;

protected:
	std::atomic<size_t> value{ 0 };
	std::chrono::time_point<std::chrono::high_resolution_clock> tpS;
	std::mutex _mtx;
	bool done = false;
	std::shared_ptr<Printer> pr = Printer::shared();

	void saveTimeS() {
		if ( tpS.time_since_epoch().count() == 0 ) tpS = std::chrono::high_resolution_clock::now();
	}

public:
	static void setColor( std::ostream &os, Color co ) {
		if ( co == Color::Grey ) os << termcolor::grey;
		if ( co == Color::Red ) os << termcolor::red;
		if ( co == Color::Green ) os << termcolor::green;
		if ( co == Color::Yellow ) os << termcolor::yellow;
		if ( co == Color::Blue ) os << termcolor::blue;
		if ( co == Color::Magenta ) os << termcolor::magenta;
		if ( co == Color::Cyan ) os << termcolor::cyan;
		if ( co == Color::White ) os << termcolor::white;
	}

	static void setFont( std::ostream &os, Style st ) {
		if ( st == Style::Bold ) os << termcolor::bold;
		if ( st == Style::Dark ) os << termcolor::dark;
		if ( st == Style::Italic ) os << termcolor::italic;
		if ( st == Style::Underline ) os << termcolor::underline;
		if ( st == Style::Blink ) os << termcolor::blink;
		if ( st == Style::Reverse ) os << termcolor::reverse;
		if ( st == Style::Concealed ) os << termcolor::concealed;
		if ( st == Style::Crossed ) os << termcolor::crossed;
	}

	size_t current() { return value; }
	bool isDone() const { return done; }
};


class Bar final : public IBar {
public:
	using FnOnChanged = std::function<void()>;

	explicit Bar( BarOpts opts = BarOpts{} ) : opts( std::move( opts ) ) {
		onChanged = [this] { this->printProgress(); };
	}

	void setOnChanged( FnOnChanged fn ) {
		std::lock_guard lock( _mtx );
		onChanged = std::move( fn );
	}

	std::ostream &getOuter() const { return *opts.ptrOuter; }
	void setOuter( std::ostream &os ) {
		std::lock_guard lock( _mtx );
		opts.ptrOuter = &os;
	}

	const BarOpts &getOpts() const { return opts; }
	void setOpts( const std::function<void( BarOpts & )> &moder ) {
		std::lock_guard lock( _mtx );
		moder( opts );
	}

	void setPrefix( const string &str ) { opts.txtL = str; }
	void setPostfix( const string &str ) { opts.txtL = str; }

	std::chrono::time_point<std::chrono::high_resolution_clock> getTimeStart() const { return tpS; }


	void setProgress( size_t v ) override {
		{
			std::lock_guard lock( _mtx );
			value = v;
			saveTimeS();
			done = value >= opts.vMax;
		}
		onChanged();
	}

	void tick() override {
		{
			std::lock_guard lock( _mtx );
			++value;
			saveTimeS();
			done = value >= opts.vMax;
		}
		onChanged();
	}

	void printProgress() override {
		std::lock_guard lock( _mtx );
		auto &os = *opts.ptrOuter;
		GuardOs guard( os );

		os << TML::CLn;


		if ( opts.color != Color::None ) setColor( os, opts.color );
		for ( const auto &style: opts.styles ) setFont( os, style );

		const auto tpN = std::chrono::high_resolution_clock::now();
		const auto useNs = std::chrono::duration_cast<std::chrono::nanoseconds>( tpN - tpS );

		const std::string fxs = opts.txtL;
		std::string fxe = opts.txtR;
		if ( opts.showPercent ) fxe.append( " " + std::to_string( std::min<size_t>( value * 100 / opts.vMax, 100 ) ) + "%" );
		if ( opts.showTimeUsed ) fxe.append( " [" + getTimeUsed( useNs ) + "]" );
		if ( opts.showTimeRemain ) fxe.append( " [" + getTimeRemain( useNs, value, opts.vMax ) + "]" );

		const size_t bs = displayWidth( fxs ) + opts.start.length();
		const size_t be = opts.width + bs;
		const size_t pos = bs + ( value - opts.vMin ) * opts.width / ( opts.vMax - opts.vMin );


		os << TML::GoH << fxs << opts.start;

		for ( size_t i = bs; i < be; ++i ) {
			if ( i < pos ) os << opts.fill;
			else if ( i == pos ) os << opts.lead;
			else os << opts.remainder;
		}
		os << opts.end << fxe;

		if ( opts.fillSpace ) {
			const size_t spaces = terminalSize().second - displayWidth( fxs + opts.start + std::string( opts.width, ' ' ) + opts.end + fxe );
			if ( spaces > 0 && spaces <= 300 ) os << std::string( spaces, ' ' );
		}

		os << TML::GoH;
		if ( done ) os << termcolor::reset;
		os.flush();
	}

private:
	BarOpts opts;
	FnOnChanged onChanged;
};


}


//------------------------------------------------------------------------
// 多個Bars的版本
//------------------------------------------------------------------------
namespace progress {

struct BarsOpts {
	std::string separator;         // 進度條之間的分隔符
	bool autoCleanFinished = true; // 是否自動清除已完成的進度條
	std::ostream *streamPtr = &std::cout;
};

class Bars {
public:
	static std::shared_ptr<Bars> shared() {
		static std::shared_ptr<Bars> instance( new Bars() );
		return instance;
	}

	void insertLog(const std::string& logText) {
		std::lock_guard<std::mutex> lock(_mtxbs);

		// 清除當前行
		(*opts.streamPtr) << TML::GoH << TML::C2E;

		// 在當前位置上方插入日誌
		(*opts.streamPtr) << TML::MvAbove << logText << std::flush;

		// 增加 cLastDraw 計數來包含新插入的行
		cLastDraw += 1;  // 如果日誌可能包含多行，這裡需要計算行數

		// 請求重繪，確保進度條正確顯示
		requestRedraw();
	}


	explicit Bars( BarsOpts opts = BarsOpts{} )
		: opts( std::move( opts ) ), cLastDraw( 0 ) {
		printer = Printer::shared();
	}

	~Bars() { stop(); }

	void add( const std::shared_ptr<Bar> &bar ) {
		std::lock_guard lock( _mtxbs );

		mapBo[bar.get()] = &bar->getOuter();

		bar->setOuter( *opts.streamPtr );
		bar->setOnChanged( [this,bar] {
			this->requestRedraw();
			if ( opts.autoCleanFinished && bar->isDone() ) { this->remove( bar ); }
		} );


		bars.push_back( bar );
		//requestRedraw();
	}

	std::shared_ptr<Bar> make( const BarOpts &barOpts = BarOpts{} ) {
		BarOpts opt = barOpts;
		opt.ptrOuter = opts.streamPtr;

		auto bar = std::make_shared<Bar>( opt );

		bar->setOnChanged( [this,bar] {
			this->requestRedraw();
			if ( opts.autoCleanFinished && bar->isDone() ) { this->remove( bar ); }
		} );

		{
			std::lock_guard lock( _mtxbs );
			bars.push_back( bar );
		}

		return bar;
	}

	void remove( const std::shared_ptr<Bar> &bar ) { remove( bar.get() ); }
	void remove( Bar *barPtr ) {
		if ( !barPtr ) return;

		std::lock_guard lock( _mtxbs );

		auto it = ranges::find_if( bars,
			[barPtr]( const std::shared_ptr<Bar> &ptr ) {
				return ptr.get() == barPtr;
			} );

		if ( it != bars.end() ) {

			const auto streamIt = mapBo.find( barPtr );
			if ( streamIt != mapBo.end() ) {
				( *it )->setOuter( *streamIt->second );
				( *it )->setOnChanged( [it] { ( *it )->printProgress(); } );
				mapBo.erase( streamIt );
			}

			bars.erase( it );
			requestRedraw();
		}
	}

	void stop() {

		{
			std::lock_guard lock( _mtxbs );
			for ( auto &bar: bars ) {
				auto streamIt = mapBo.find( bar.get() );
				if ( streamIt != mapBo.end() ) {

					bar->setOuter( *streamIt->second );
					bar->setOnChanged( [bar] { bar->printProgress(); } );
				}
			}

			mapBo.clear();
			if ( !bars.empty() ) bars.clear();
		}

		if ( !bars.empty() ) requestClearLines();
	}

	void setOpts( const std::function<void( BarsOpts & )> &modifier ) {
		std::lock_guard lock( _mtxbs );
		modifier( opts );
		requestRedraw();
	}

	size_t size() const {
		std::lock_guard lock( _mtxbs );
		return bars.size();
	}

	bool empty() const {
		std::lock_guard lock( _mtxbs );
		return bars.empty();
	}

private:
	mutable std::mutex _mtxbs;
	BarsOpts opts;
	std::vector<std::shared_ptr<Bar>> bars;
	std::map<Bar *, std::ostream *> mapBo;
	std::shared_ptr<Printer> printer;
	size_t cLastDraw; // 上次繪製的行數

	void requestRedraw() {

		std::vector<std::shared_ptr<Bar>> barsCopy = bars;
		BarsOpts optsCopy = opts;

		printer->addJob( [this, barsCopy, optsCopy] {
			size_t newLineCount = drawBars( barsCopy, optsCopy, cLastDraw );

			std::lock_guard lock( _mtxbs );
			cLastDraw = newLineCount;
		} );
	}

	void requestClearLines() {
		size_t cLastLs = cLastDraw;
		std::ostream *cpOS = opts.streamPtr;

		printer->addJob( [this, cLastLs, cpOS] {
			auto &os = *cpOS;
			for ( size_t i = 0; i < cLastLs; ++i ) {
				os << TML::CLn;                         // 清除當前行
				if ( i < cLastLs - 1 ) os << TML::MvUp; // 上移一行
			}

			os << std::endl;
			os.flush();

			std::lock_guard lock( _mtxbs );
			cLastDraw = 0;
		} );

		*cpOS << std::endl << std::flush;
	}

	static size_t drawBars( const std::vector<std::shared_ptr<Bar>> &bars,
		const BarsOpts &opts,
		size_t cLastLs ) {
		auto &os = *opts.streamPtr;

		if ( cLastLs > 0 ) {
			os << TML::GoH;                                         // 移到第一行的開始
			for ( size_t i = 1; i < cLastLs; ++i ) os << TML::MvUp; // 向上移動一行

			for ( size_t i = 0; i < cLastLs; ++i ) {
				os << TML::C2E;
				if ( i < cLastLs - 1 ) os << TML::NewL;
			}

			os << TML::GoH;
			for ( size_t i = 1; i < cLastLs; ++i ) os << TML::MvUp;
		}

		size_t cLines = 0;
		for ( size_t i = 0; i < bars.size(); ++i ) {
			cLines++;                                                       // 進度條佔行
			if ( i < bars.size() - 1 && !opts.separator.empty() ) cLines++; // 分隔符佔行
		}

		for ( size_t i = 0; i < bars.size(); ++i ) {
			GuardOs guard( os );

			const auto &bar = bars[i];
			const auto &bos = bar->getOpts();

			if ( bos.color != Color::None ) IBar::setColor( os, bos.color );
			for ( const auto &st: bos.styles ) IBar::setFont( os, st );

			const size_t value = bar->current();
			auto now = std::chrono::high_resolution_clock::now();
			auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>( now - bar->getTimeStart() );

			std::string txL = bos.txtL;
			std::string txR = bos.txtR;

			if ( bos.showPercent ) txR.append( " " + std::to_string( std::min<size_t>( value * 100 / bos.vMax, 100 ) ) + "%" );
			if ( bos.showTimeUsed ) txR.append( " [" + getTimeUsed( elapsed ) + "]" );
			if ( bos.showTimeRemain ) txR.append( " [" + getTimeRemain( elapsed, value, bos.vMax ) + "]" );

			const size_t bs = displayWidth( txL ) + bos.start.length();
			const size_t be = bos.width + bs;
			const size_t pos = bs + ( value - bos.vMin ) * bos.width / ( bos.vMax - bos.vMin );

			os << txL << bos.start;

			for ( size_t j = bs; j < be; ++j ) {
				if ( j < pos ) os << bos.fill;
				else if ( j == pos ) os << bos.lead;
				else os << bos.remainder;
			}

			os << bos.end << txR;

			// 如果不是最後一個進度條且有分隔符
			if ( i < bars.size() - 1 ) {
				if ( !opts.separator.empty() ) os << TML::NewL << opts.separator;
				os << TML::NewL; // 每個進度條後換行
			}
		}

		os.flush();
		return cLines;
	}

};



}
