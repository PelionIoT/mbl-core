/*
 * Copyright (c) 2020 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "sig.h"

namespace updated {
namespace signal {

extern "C" void sigint_handler(int sig)
{
    sigint = sig;
}

extern "C" void sighup_handler(int sig)
{
    sighup = sig;
}

void register_handlers()
{
    std::signal(SIGINT, sigint_handler);
    std::signal(SIGHUP, sighup_handler);
}


} // namespace signal
} // namespace updated
