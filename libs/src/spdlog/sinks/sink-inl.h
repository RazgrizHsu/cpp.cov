// Copyright(c) 2015-present, Gabi Melman & lg contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#ifndef SPDLOG_HEADER_ONLY
#include <lg/sinks/sink.h>
#endif

#include <spdlog/common.h>

SPDLOG_INLINE bool lg::sinks::sink::should_log( lg::level::level_enum msg_level) const
{
    return msg_level >= level_.load(std::memory_order_relaxed);
}

SPDLOG_INLINE void lg::sinks::sink::set_level( level::level_enum log_level)
{
    level_.store(log_level, std::memory_order_relaxed);
}

SPDLOG_INLINE lg::level::level_enum lg::sinks::sink::level() const
{
    return static_cast<lg::level::level_enum>(level_.load(std::memory_order_relaxed));
}
