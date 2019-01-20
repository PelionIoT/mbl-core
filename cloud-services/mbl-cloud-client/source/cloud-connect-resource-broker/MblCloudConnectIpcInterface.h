/*
 * Copyright (c) 2019 ARM Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef MblCloudConnectIpcInterface_h_
#define MblCloudConnectIpcInterface_h_

#include "MblError.h"

namespace mbl {

/*! \file MblCloudConnectIpcInterface.h
 *  \brief MblCloudConnectIpcInterface.
 *  This class provides an interface for all IPC mechanisms to allow applications to register their own LwM2M resources 
 *  with the Mbed Cloud for application specific purposes.
*/
class MblCloudConnectIpcInterface {

public:

    MblCloudConnectIpcInterface() = default;
    virtual ~MblCloudConnectIpcInterface() = default;
    
    // initialize IPC instance
    // return Error::None if succeeded, error TBD error code otherwise
    virtual MblError init() = 0;

    // Runs IPC event-loop
    // return Error::None if succeeded, error TBD error code otherwise
    virtual MblError run() = 0;

    // The caller thread will join with the IPC thread
    // param[in] args - output parameter that will contain thread output data
    // return Error::None if succeeded, Error::ThreadJoiningFailed otherwise
    virtual MblError thread_join(void **args) = 0;

    // Signals to the IPC thread that it should finish ASAP
    // return Error::None if succeeded, Error::ThreadFinishingFailed otherwise
    virtual MblError thread_finish() = 0;

private:

    // No copying or moving (see https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#cdefop-default-operations)
    MblCloudConnectIpcInterface(const MblCloudConnectIpcInterface&) = delete;
    MblCloudConnectIpcInterface & operator = (const MblCloudConnectIpcInterface&) = delete;
    MblCloudConnectIpcInterface(MblCloudConnectIpcInterface&&) = delete;
    MblCloudConnectIpcInterface& operator = (MblCloudConnectIpcInterface&&) = delete;    
};

} // namespace mbl

#endif // MblCloudConnectIpcInterface_h_
