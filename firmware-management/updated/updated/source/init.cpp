/*
 * Copyright (c) 2020 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * This module contains initialisation functions and state.
 */

#include "init.h"
#include "logging/logger.h"
#include "signal.h"

#include <systemd/sd-daemon.h>

namespace updated {
namespace init {

Status initialise(const InitData &init_data)
{
    logging::create_systemd_logger(
        logging::level_from_string(init_data.log_level)
    );
    signal::register_handlers();
    return Status::Started;
}

void notify_start(const Status startup_status)
{
    switch(startup_status)
    {
        case Status::Started:
            sd_notify(0, "READY=1");
            break;

        case Status::FailedToStart:
            sd_notify(0, "STATUS=Failed to start\n");
            break;
    }
}

} // namespace init
} // namespace updated
