/*
 * Copyright (c) 2020 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UPDATED_LOGGING_H
#define UPDATED_LOGGING_H

#include "spdlog/spdlog.h"
#include "spdlog/sinks/systemd_sink.h"

#include <memory>
#include <string_view>

namespace updated {
namespace logging {

/* Create aliases for spdlog levels in our namespace */
using spdlog::trace;
using spdlog::info;
using spdlog::debug;
using spdlog::error;
using spdlog::critical;
using spdlog::warn;

enum class Level
{
    CRITICAL,
    ERROR,
    WARN,
    INFO,
    DEBUG,
    TRACE
};

/**
 * Takes a string log level and returns a logging::Level.
 *
 * Valid levels are: CRITICAL, ERROR, WARNING, INFO, DEBUG or TRACE
 */
Level level_from_string(const std::string_view str);

/**
 * Creates a global logger with a systemd log sink.
 */
std::shared_ptr<spdlog::logger> create_systemd_logger(Level level);

/**
 * Get a shared_ptr to the global logger.
 */
std::shared_ptr<spdlog::logger> get_logger();

} // namespace logging
} // namespace updated


#endif // UPDATED_LOGGING_H
