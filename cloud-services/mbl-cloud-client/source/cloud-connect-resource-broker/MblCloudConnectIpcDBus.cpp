/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "MblCloudConnectIpcDBus.h"

#include "mbed-trace/mbed_trace.h"

#define TRACE_GROUP "CCRB"

namespace mbl {

MblCloudConnectIpcDBus::MblCloudConnectIpcDBus()
{
    tr_info("MblCloudConnectIpcDBus::MblCloudConnectIpcDBus");
}

MblCloudConnectIpcDBus::~MblCloudConnectIpcDBus()
{
    tr_info("MblCloudConnectIpcDBus::~MblCloudConnectIpcDBus");
}

} // namespace mbl
