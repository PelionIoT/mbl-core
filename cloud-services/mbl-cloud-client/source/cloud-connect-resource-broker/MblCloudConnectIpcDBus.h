/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MblCloudConnectIpcDBus_h_
#define MblCloudConnectIpcDBus_h_

#include "MblCloudConnectIpcInterface.h"

namespace mbl {

/*! \file MblCloudConnectIpcDBus.h
 *  \brief MblCloudConnectIpcDBus.
 *  This class provides an implementation for D-Bus IPC mechanism
 */
class MblCloudConnectIpcDBus: public MblCloudConnectIpcInterface {

public:

    MblCloudConnectIpcDBus();
    virtual ~MblCloudConnectIpcDBus();

};

} // namespace mbl

#endif // MblCloudConnectIpcDBus_h_
