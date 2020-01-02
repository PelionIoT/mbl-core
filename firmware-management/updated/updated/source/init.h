/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * This module contains initialisation functions and state
 */
#ifndef UPDATED_INIT_H
#define UPDATED_INIT_H

namespace updated {
namespace init {

enum class Status
{
    Started,
    FailedToStart,
};

/** Initialise UpdateD.
 *
 *  This is where we add our signal handlers, initialise our logging
 *  mechanism and perform any other startup housekeeping.
 *
 *  Returns init::Status, which is used to notify the init system.
 */
Status initialise();

/** Notify systemd that UpdateD started.
 *
 *  Take an init::Status and use it to notify systemd of the startup status.
 */
void notify_start(Status startup_status);

} // namespace init
} // namespace updated

#endif // UPDATED_INIT_H
