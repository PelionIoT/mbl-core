/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MblCloudConnectResourceBroker_h_
#define MblCloudConnectResourceBroker_h_

#include "mbed-cloud-client/MbedCloudClient.h"
#include "MblCloudConnectIpcDBus.h"

#include "../MblError.h"
#include "../MblMutex.h"

namespace mbl {

class MblCloudConnectResourceBroker {

public:

    MblCloudConnectResourceBroker();
    virtual ~MblCloudConnectResourceBroker();

private:

    MblCloudConnectIpcDBus mIpcDBus;
 
};

} // namespace mbl

#endif // MblCloudConnectResourceBroker_h_
