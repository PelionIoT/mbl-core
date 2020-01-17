/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * UpdateD (pronounced update-dee) is a system daemon that coordinates updates
 * to device firmware. Updates will consist of bundles of one or more
 * firmware packages that should be applied as a set. The delivery of updates
 * to a device will be the responsibility of an external delivery mechanism.
 * UpdateD runs in the background on a device and listens for update requests.
 * When a request is received, UpdateD will hand off responsibility for applying
 * the update to swupdate.
 */

#include "cli.h"
#include "init.h"
#include "signal.h"
#include "UpdateCoordinator.h"
#include "logging/logger.h"

#include "rpc/Server.h"

#include <string>
#include <stdexcept>
#include <unistd.h>

int main(int argc, char** argv)
{
    std::string log_level;

    try
    {
        log_level = updated::cli::parse_args(argc, argv);
    }
    catch (std::invalid_argument &e)
    {
        std::cerr << e.what() << '\n';
        updated::init::notify_start(updated::init::Status::FailedToStart);
        return 1;
    }

    updated::UpdateCoordinator update_coordinator;
    updated::rpc::Server rpc_server{update_coordinator};
    const updated::init::Status init_status = updated::init::initialise({log_level});
    updated::init::notify_start(init_status);

    while(!updated::signal::sigint)
    {
        update_coordinator.run();
    }

    return 0;
}
