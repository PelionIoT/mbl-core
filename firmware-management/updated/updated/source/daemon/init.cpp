/*
 * Copyright (c) 2020 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * This module contains initialisation functions and state.
 */

#include "init.h"

#include "../logging/logger.h"
#include "../signal/handlers.h"

#include <systemd/sd-daemon.h>

namespace updated {
namespace init {

DaemonInitialiser::DaemonInitialiser(const InitData &init_data)
    : instance_lock{"/tmp/updated_lock"}
{
    const Status init_status = initialise(init_data);
    notify_start(init_status);
}

DaemonInitialiser::Status DaemonInitialiser::initialise(const InitData &init_data)
{
    logging::create_systemd_logger(
        logging::level_from_string(init_data.log_level)
    );
    signal::register_handlers();
    return Status::Started;
}

void DaemonInitialiser::notify_start(const DaemonInitialiser::Status startup_status)
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
