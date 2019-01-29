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
#include "MblCloudConnectTypes.h"

#include  <memory>


namespace mbl {

/**
 * @brief Class implements functionality of Mbl Cloud Connect Resource 
 * Broker (CCRB). Main functionality: 
 * - receive and manage requests from applications to MbedCloudClient.
 * - send observers notifications from MbedCloudClient to applications.
 */
class MblCloudConnectResourceBroker {

public:
    MblCloudConnectResourceBroker();
    ~MblCloudConnectResourceBroker();

/////////////////////////////////////////////////////////////////////
// MblCloudConnectResourceBroker to MblCloudClient API
/////////////////////////////////////////////////////////////////////

/**
 * @brief Starts CCRB.
 * In details: 
 * - initializes CCRB instance and runs event-loop.
 * @return MblError returns value Error::None if function succeeded, 
 *         or Error::CCRBStartFailed otherwise. 
 */
    MblError start();

/**
 * @brief Stops CCRB.
 * In details: 
 * - stops CCRB event-loop.
 * - deinitializes CCRB instance.
 * 
 * @return MblError returns value Error::None if function succeeded, 
 *         or Error::CCRBStopFailed otherwise. 
 */
    MblError stop();



/////////////////////////////////////////////////////////////////////
// MblCloudConnectResourceBroker to MblCloudConnectIpcDBus API
/////////////////////////////////////////////////////////////////////

/**
 * @brief Starts registration request of the resource set in the Cloud.  
 * This function parses the input json file, and creates resource objects 
 * from it. Created objects pends for the registration to the Cloud. 
 * CCRB will send the final status of the registration to the application 
 * (when it will be ready) by calling update_registration_status API. 
 * @param appl_context is a parameter that will be returned to the client 
 * application in update_registration_status API.
 * @param ipc_conn_handle is a handle to the IPC unique connection information 
 *        of the application that should get update_registration_status message.
 * @param appl_resource_definition_json is a json file that describes resources 
 *        that should be registered. The structure of the JSON document 
 *        reflects the structure of the required resource tree. 
 * @return MblError returns success if json file successfully
 *         parsed and the registration request was sent to the Cloud for 
 *         the processing, or error code otherwise. 
 */
    MblError register_resources_start(
        const uintptr_t appl_context, 
        const uintptr_t ipc_conn_handle, 
        const std::string &appl_resource_definition_json);

/**
 * @brief Starts deregistration request of the resource set from the Cloud.  
 * This function starts deregistration procedure of all resources that are
 * "owned" by access_token. CCRB will send the final status of the 
 * deregistration to the application (when it will be ready) by calling 
 * update_deregistration_status API. 
 * @param appl_context is a parameter that will be returned to the client 
 * application in update_deregistration_status API.
 * @param ipc_conn_handle is a handle to the IPC unique connection information 
 *        of the application that should be notified.
 * @param access_token is a token that defines set of resources that should be 
 *        deregistered.   
 * @return MblError returns success if the deregistration request 
 *         was sent to the Cloud for the processing, or error code otherwise. 
 */
    MblError deregister_resources_start(
        const uintptr_t appl_context, 
        const uintptr_t ipc_conn_handle, 
        const std::string access_token);

// FIXME
    MblError add_resource_instances_start(
        const uintptr_t appl_context, 
        const uintptr_t ipc_conn_handle, 
        const std::string access_token, 
        const std::string &resource_path, 
        const std::vector<uint16_t> &resource_instance_ids);

// FIXME
    MblError remove_resource_instances_start(
        const uintptr_t appl_context, 
        const uintptr_t ipc_conn_handle, 
        const std::string access_token, 
        const std::string &resource_path, 
        const std::vector<uint16_t> &resource_instance_ids);

// FIXME
    MblError set_resource_values(
        const std::string access_token, 
        const std::vector<MblCloudConnect_ResourcePath_Value> &input_values, 
        std::vector<MblCloudConnect_ResourcePath_Status> &output_set_statuses);

// FIXME
    MblError get_resource_values(
        const std::string access_token, 
        const std::vector<MblCloudConnect_ResourcePath_Type> &input_paths,
        std::vector<MblCloudConnect_ResourcePath_Value_Status> &output_get_values);

private:

/**
 * @brief Initializes CCRB instance.
 * 
 * @return MblError returns value Error::None if function succeeded, 
 *         or error code otherwise.
 */
    MblError init();

/**
 * @brief Deinitializes CCRB instance.
 * 
 * @return MblError returns value Error::None if function succeeded, 
 *         or error code otherwise.
 */
    MblError de_init();

/**
 * @brief Runs CCRB event-loop.
 * 
 * @return MblError returns value Error::None if function succeeded, 
 *         or error code otherwise.
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
