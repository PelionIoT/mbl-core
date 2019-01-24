/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
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


#ifndef MblCloudConnectResourceBroker_h_
#define MblCloudConnectResourceBroker_h_

#include "MblCloudConnectIpcInterface.h"

#include  <memory>

namespace mbl {

/**
 * @brief Class implements functionality of Mbl Cloud Connect Resource Broker (CCRB). 
 * Main functionality: 
 * - receive and manage requests from applications to MbedCloudClient.
 * - send observers notifications from MbedCloudClient to applications.
 */
class MblCloudConnectResourceBroker {

public:

    MblCloudConnectResourceBroker();
    ~MblCloudConnectResourceBroker();

/**
 * @brief Starts CCRB.
 * In details: 
 * - initializes CCRB instance and runs event-loop.
 * @return MblError returns value Error::None if function succeeded, or Error::CCRBStartFailed otherwise. 
 */
    MblError start();

/**
 * @brief Stops CCRB.
 * In details: 
 * - stops CCRB event-loop.
 * - deinitializes CCRB instance.
 * 
 * @return MblError returns value Error::None if function succeeded, or Error::CCRBStopFailed otherwise. 
 */
    MblError stop();

private:

/**
 * @brief Initializes CCRB instance.
 * 
 * @return MblError returns value Error::None if function succeeded, or error code otherwise.
 */
    MblError init();

/**
 * @brief Deinitializes CCRB instance.
 * 
 * @return MblError returns value Error::None if function succeeded, or error code otherwise.
 */
    MblError de_init();

/**
 * @brief Runs CCRB event-loop.
 * 
 * @return MblError returns value Error::None if function succeeded, or error code otherwise.
 */
    MblError run();

/**
 * @brief CCRB thread main function.
 * In details: 
 * - initializes CCRB module.
 * - runs CCRB main functionality loop.  
 * 
 * @param ccrb address of CCRB instance that should run. 
 * @return void* thread output buffer - not used.
 */
    static void *ccrb_main(void *ccrb);

private:

    // No copying or moving (see https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#cdefop-default-operations)
    MblCloudConnectResourceBroker(const MblCloudConnectResourceBroker&) = delete;
    MblCloudConnectResourceBroker & operator = (const MblCloudConnectResourceBroker&) = delete;
    MblCloudConnectResourceBroker(MblCloudConnectResourceBroker&&) = delete;
    MblCloudConnectResourceBroker& operator = (MblCloudConnectResourceBroker&&) = delete;  

    // thread id of the IPC thread
    pthread_t ipc_thread_id_ = 0;

    // pointer to ipc binder instance
    std::unique_ptr<MblCloudConnectIpcInterface> ipc_ = nullptr;
};

} // namespace mbl

#endif // MblCloudConnectResourceBroker_h_
