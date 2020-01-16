/*
 * Copyright (c) 2020 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "logger.h"

#include <string_view>

namespace updated {
namespace logging {

namespace {

constexpr auto LOGGER_NAME = "systemd";

void set_global_level(Level level)
{
    switch (level)
    {
    case Level::CRITICAL:
        spdlog::set_level(spdlog::level::critical);
        break;
    case Level::ERROR:
        spdlog::set_level(spdlog::level::err);
        break;
    case Level::WARN:
        spdlog::set_level(spdlog::level::warn);
        break;
    case Level::INFO:
        spdlog::set_level(spdlog::level::info);
        break;
    case Level::DEBUG:
        spdlog::set_level(spdlog::level::debug);
        break;
    case Level::TRACE:
        spdlog::set_level(spdlog::level::trace);
        break;
    }
}

} // anonymous namespace

Level level_from_string(const std::string_view str)
{
    if (str == "CRITICAL") { return Level::CRITICAL; }
    if (str == "ERROR") { return Level::ERROR; }
    if (str == "WARNING") { return Level::WARN; }
    if (str == "INFO") { return Level::INFO; }
    if (str == "DEBUG") { return Level::DEBUG; }
    if (str == "TRACE") { return Level::TRACE; }

    throw std::invalid_argument("Unrecognized string level.");
}

std::shared_ptr<spdlog::logger> create_systemd_logger(Level level)
{
    auto logger = spdlog::systemd_logger_mt("systemd"); // NOLINT: spdlog misuses std::move, disable checking here
    set_global_level(level);
    return logger;
}

std::shared_ptr<spdlog::logger> get_logger()
{
    return spdlog::get(LOGGER_NAME);
}

} // namespace logging
} // namespace updated


