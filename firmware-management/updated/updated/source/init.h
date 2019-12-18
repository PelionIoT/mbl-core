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

        Status initialise();
        void notify_start(Status startup_status);
    } // namespace init
} // namespace updated

#endif // UPDATED_INIT_H
