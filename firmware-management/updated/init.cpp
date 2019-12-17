/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * This module contains initialisation functions and state.
 */

#include "init.h"

#include <systemd/sd-daemon.h>


namespace updated {
    namespace init {
        Status initialise()
        {
            return Status::Started;
        }

        // Notify the init system that we started successfully
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
