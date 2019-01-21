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

/**
 * @brief Mbl Cloud Connect Ipc Interface.
 * This class provides an interface for all IPC mechanisms that allow comminication of CCRB module with OS IPC mechanism.
 * 
 */
class MblCloudConnectIpcInterface {

public:

    MblCloudConnectIpcInterface() = default;
    virtual ~MblCloudConnectIpcInterface() = default;
    
/**
 * @brief Initialize IPC instance.
 * 
 * @return MblError has value Error::None if function succeeded, or error code otherwise.
 */
    virtual MblError init() = 0;

/**
 * @brief Runs IPC event-loop.
 * 
 * @return MblError has value Error::None if function succeeded, or error code otherwise.
 */
    virtual MblError run() = 0;

/**
 * @brief Function joins the caller thread with the IPC thread.
 * 
 * @param args output parameter that will contain thread output data. args can be NULL.
 * @return MblError has value Error::None if function succeeded, or Error::ThreadJoiningFailed otherwise.
 */
    virtual MblError thread_join(void **args) = 0;

/**
 * @brief Signals to the IPC thread that it should finish ASAP
 * 
 * @return MblError has value Error::None if function succeeded, or Error::ThreadFinishingFailed otherwise.
 */
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
