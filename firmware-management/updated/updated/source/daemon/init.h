/*
 * Copyright (c) 2020 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * This module contains initialisation functions and state
 */

#ifndef UPDATED_INIT_H
#define UPDATED_INIT_H

#include "../fileutils/LockFile.h"

#include <string>

namespace updated {
namespace init {

/**
 * InitData contains initialisation state for UpdateD
 */
struct InitData
{
    std::string log_level;

    InitData(std::string log_lvl)
        : log_level{log_lvl}
    {}
};

/**
 * This object handles any initialisation the UpdateD daemon needs.
 *
 * Acquires a lock file, adds signal handlers and sets the log level.
 */
class DaemonInitialiser final
{
public:
    explicit DaemonInitialiser(const InitData &init_data);
    ~DaemonInitialiser() = default;

    enum class Status
    {
        Started,
        FailedToStart,
    };

private:
    /**
     * Initialise UpdateD.
     *
     * This is where we add our signal handlers, initialise our logging
     * mechanism and perform any other startup housekeeping.
     *
     * Returns init::Status, which is used to notify the init system.
     */
    static Status initialise(const InitData &init_data);

    /**
     * Notify systemd that UpdateD started.
     *
     * Take an init::Status and use it to notify systemd of the startup status.
     */
    static void notify_start(Status startup_status);

    fileutils::LockFile instance_lock;
};

} // namespace init
} // namespace updated

#endif // UPDATED_INIT_H
