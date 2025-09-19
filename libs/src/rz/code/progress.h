#pragma once

#include <functional>
#include <rz/@lib/progress.hpp>
#include <spdlog/sinks/base_sink.h>
#include <mutex>


namespace rz::code::prog {

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


static bool g_progress_active = false;

// 與進度條協調的自定義 sink
template<typename Mutex>
class sinkProgress : public lg::sinks::base_sink<Mutex> {
protected:
	void sink_it_(const lg::details::log_msg &msg) override {

		lg::memory_buf_t formatted;
		this->formatter_->format(msg, formatted);
		std::string output = fmt::to_string(formatted);

		if (g_progress_active) {
			// 在上方插入日誌

			// 通知 Bars 類有日誌插入
			if (auto bars = progress::Bars::shared()) {
				if ( bars->size() >= 1 ) {
					bars->insertLog(output);
					return;
				}
			}

			std::cout << "\n\033[A\033[1L" << "Bars?? " << output << std::flush;
		}
		else {
			std::cout << output << std::flush;
		}
	}

	void flush_() override {
		std::cout << std::flush;
	}
};

using progress_compatible_sink_mt = sinkProgress<std::mutex>;
using progress_compatible_sink_st = sinkProgress<lg::details::null_mutex>;

// 為預認 logger 添加自定義 sink
inline void setCompatibleLogger( bool active = true ) {
	auto sink = std::make_shared<progress_compatible_sink_mt>();
	auto logger = lg::default_logger();

	logger->sinks().clear();
	logger->sinks().push_back( sink );

	if ( active ) { g_progress_active = true; }
}

}
