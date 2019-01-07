/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MblCloudConnectIpcInterface_h_
#define MblCloudConnectIpcInterface_h_

namespace mbl {

/*! \file MblCloudConnectIpcInterface.h
 *  \brief MblCloudConnectIpcInterface.
 *  This class provides an interface for all IPC mechanisms
 */
class MblCloudConnectIpcInterface {

public:

    virtual ~MblCloudConnectIpcInterface(){};

protected:

    MblCloudConnectIpcInterface(){}
    MblCloudConnectIpcInterface(const MblCloudConnectIpcInterface & ) {}
    MblCloudConnectIpcInterface & operator = (const MblCloudConnectIpcInterface & ) { return *this ; }
};

} // namespace mbl

#endif // MblCloudConnectIpcInterface_h_
