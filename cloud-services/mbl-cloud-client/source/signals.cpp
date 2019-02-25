/*
 * Copyright (c) 2017 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "signals.h"

#include "MblCloudClient.h"
#include "log.h"

#include "mbed-trace/mbed_trace.h"

#include <cerrno>
#include <cstring>
#include <signal.h>

#define TRACE_GROUP "mbl"

namespace mbl
{

MblError signals_init()
{
    // Shutdown
    struct sigaction shutdown_sa;
    std::memset(&shutdown_sa, 0, sizeof(shutdown_sa));
    shutdown_sa.sa_handler = mbl_shutdown_handler;
    shutdown_sa.sa_flags = 0;
    if (sigaction(SIGTERM, &shutdown_sa, 0) != 0) {
        tr_error("Failed to set SIGTERM signal handler: %s", std::strerror(errno));
        return Error::SignalsInitSigaction;
    }
    if (sigaction(SIGINT, &shutdown_sa, 0) != 0) {
        tr_error("Failed to set SIGINT signal handler: %s", std::strerror(errno));
        return Error::SignalsInitSigaction;
    }

    // Log reopen
    struct sigaction log_sa;
    std::memset(&log_sa, 0, sizeof(log_sa));
    log_sa.sa_handler = mbl_log_reopen_signal_handler;
    log_sa.sa_flags = SA_RESTART;
    if (sigaction(SIGHUP, &log_sa, 0) != 0) {
        tr_error("Failed to set SIGHUP signal handler: %s", std::strerror(errno));
        return Error::SignalsInitSigaction;
    }

    return Error::None;
}

} // namespace mbl
