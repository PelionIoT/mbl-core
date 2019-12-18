#ifndef UPDATED_LOGGER_H
#define UPDATED_LOGGER_H

#include <spdlog/spdlog.h>
#include <spdlog/sinks/systemd_sink.h>


namespace updated
{
    using logger = spdlog;
    auto systemd_sink = std::make_shared<logger::sinks::systemd_sink_st>();
    logger::logger logger("updated_logger", systemd_sink);
    logger::set_default_logger(logger);
}

#endif // UPDATED_LOGGER_H
