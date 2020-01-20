/*
 * Copyright (c) 2020 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UPDATED_SIGNAL_H
#define UPDATED_SIGNAL_H

#include <csignal>

namespace updated{
namespace signal {

static volatile std::sig_atomic_t sighup{0};
static volatile std::sig_atomic_t sigint{0};

/**
 * Register the signal handlers.
 */
void register_handlers();

/**
 * SIGINT handler.
 *
 * This handler writes to std::sig_atomic_t sigint, which causes a clean shutdown.
 */
extern "C" void sigint_handler(int sig);

/**
 * SIGHUP handler.
 *
 * Writes to std::sig_atomic_t sighup, which has no effect at the moment.
 * SIGHUP would generally mean we should reload config files. We don't have
 * any config to reload at the moment so we do nothing.
 */
extern "C" void sighup_handler(int sig);

} // namespace signal
} // namespace updated


#endif // UPDATED_SIGNAL_H
