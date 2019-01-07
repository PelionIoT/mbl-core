/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "MblCloudConnectResourceBroker.h"

#include "mbed-trace/mbed_trace.h"

#define TRACE_GROUP "CCRB"

namespace mbl {

MblCloudConnectResourceBroker::MblCloudConnectResourceBroker()
{
    tr_info("MblCloudConnectResourceBroker::MblCloudConnectResourceBroker");
}

MblCloudConnectResourceBroker::~MblCloudConnectResourceBroker()
{
    tr_info("MblCloudConnectResourceBroker::~MblCloudConnectResourceBroker");
}

} // namespace mbl
