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
 * 
 */
class MblCloudConnectResourceBroker {

public:

    // Currently, this constructor is called from MblCloudClient thread.
    // Might change in future. If we shall need to call the constructor from the
    // ccrb thread, change MblCloudClient::cloud_connect_resource_broker_ instance
    // member to be a pointer and call the constructor from ccrb thread thread_function
    MblCloudConnectResourceBroker();
    ~MblCloudConnectResourceBroker();

/**
 * @brief Initializes CCRB instance. 
 * In details: 
 * - calls IPC module init function.  
 * 
 * @return MblError has value Error::None if function succeeded, or error code otherwise. 
 */
    MblError init();

/**
 * @brief Runs CCRB module's main functionality loop. 
 * In details: 
 * - runs IPC module's main functionality loop.  
 * 
 * @return MblError has value Error::None if function succeeded, or error code otherwise. 
 */
    MblError run();


/**
 * @brief Function joins the caller thread with the IPC thread. 
 * 
 * @param args output parameter that will contain thread output data. args can be NULL.
 * @return MblError has value Error::None if function succeeded, or Error::ThreadJoiningFailed otherwise.
 */
    MblError thread_join(void **args);

 /**
 * @brief Signals to the IPC thread that it should finish ASAP.
 * 
 * @return MblError has value Error::None if function succeeded, or Error::ThreadFinishingFailed otherwise.
 */   
    MblError thread_finish();

/**
 * @brief Thread main function.
 * In details: 
 * - initializes CCRB module.
 * - runs CCRB main functionality loop.  
 * 
 * @param ccrb_instance_ptr address of CCRB instance that should run. 
 * @return void* thread output buffer.  
 */
    static void *thread_function(void *ccrb_instance_ptr);

private:

    std::unique_ptr<MblCloudConnectIpcInterface> ipc_;
};

} // namespace mbl

#endif // MblCloudConnectResourceBroker_h_
