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
// API to be used by MblCloudClient class
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
// API to be used by MblCloudConnectIpcInterface derivaed class 
/////////////////////////////////////////////////////////////////////

/**
 * @brief Starts asynchronous registration request of the resource set 
 * in the Cloud.  
 * This function parses the input json file, and creates resource objects 
 * from it. Created objects pends for the registration to the Cloud. 
 * CCRB will send the final status of the registration to the application 
 * (when it will be ready) by calling update_registration_status API. 
 * 
 * @param ipc_conn_handle handle to the IPC unique connection information 
 *        of the application that should get update_registration_status message.
 * @param appl_resource_definition_json json file that describes resources 
 *        that should be registered. The structure of the JSON document 
 *        reflects the structure of the required resource tree. 
 * @return MblError returns Error::None if json file successfully
 *         parsed and the registration request was sent to the Cloud for 
 *         the processing, or error code otherwise. 
 */
    MblError register_resources_async(
        const uintptr_t ipc_conn_handle, 
        const std::string &appl_resource_definition_json);

/**
 * @brief Starts asynchronous deregistration request of the resource set 
 * from the Cloud.  
 * This function starts deregistration procedure of all resources that are
 * "owned" by access_token. CCRB will send the final status of the 
 * deregistration to the application (when it will be ready) by calling 
 * update_deregistration_status API. 
 * 
 * @param ipc_conn_handle handle to the IPC unique connection information 
 *        of the application that should be notified.
 * @param access_token token that defines set of resources that should be 
 *        deregistered.   
 * @return MblError returns Error::None if the deregistration request 
 *         was sent to the Cloud for the processing, or error code otherwise. 
 */
    MblError deregister_resources_async(
        const uintptr_t ipc_conn_handle, 
        const std::string &access_token);

/**
 * @brief Starts resource instances addition asynchronous request to 
 * the Cloud.  
 * This function starts resource instances addition procedure of all 
 * resources instances that are provided in resource_instance_ids. 
 * CCRB will send the final status of the resource instances addition 
 * to the application (when it will be ready) by calling 
 * update_add_resource_instance_status API.
 * 
 * @param ipc_conn_handle handle to the IPC unique connection information 
 *        of the application that should be notified.
 * @param access_token token used for access control to the resource which path 
 *        is provided in resource_path argument.
 * @param resource_path path of the resource to which instances 
 *        should be added.  
 * @param resource_instance_ids vector of instance ids. Each instance id 
 *        in the vector is an id of the resource instance that should be 
 *        added to the given resource (identified by resource_path).   
 * @return MblError returns Error::None if the resource instances addition 
 *         request was sent to the Cloud for the processing, or error 
 *         code otherwise.
 */
    MblError add_resource_instances_async(
        const uintptr_t ipc_conn_handle, 
        const std::string &access_token, 
        const std::string &resource_path, 
        const std::vector<uint16_t> &resource_instance_ids);

/**
 * @brief Starts resource instances remove asynchronous request 
 * from the Cloud.  
 * This function starts resource instances remove procedure of all 
 * resources instances that are provided in resource_instance_ids. 
 * CCRB will send the final status of the resource instances removal 
 * to the application (when it will be ready) by calling 
 * update_remove_resource_instance_status API.
 * 
 * @param ipc_conn_handle handle to the IPC unique connection information 
 *        of the application that should be notified.
 * @param access_token token used for access control to the resource which path 
 *        is provided in resource_path argument.
 * @param resource_path path of the resource from which instances 
 *        should be removed.  
 * @param resource_instance_ids vector of instance ids. Each instance id 
 *        in the vector is an id of the resource instance that should be 
 *        removed from the given resource (identified by resource_path).   
 * @return MblError returns Error::None if the resource instances removal 
 *         request was sent to the Cloud for the processing, or error 
 *         code otherwise.
 */
    MblError remove_resource_instances_async(
        const uintptr_t ipc_conn_handle, 
        const std::string &access_token, 
        const std::string &resource_path, 
        const std::vector<uint16_t> &resource_instance_ids);

/**
 * @brief Set resource values for multiple resources. 
 *
 * @param access_token token used for access control to the resources pointed 
 *        from input_values vector. 
 * @param input_values vector of tuples from the format
 *        [resource_path, resource_typed_data_value]. Each tuple in vector defines
 *        path of the resource who's value should be set to the input value.
 * @param output_set_statuses output vector of tuples from the format
 *        [resource_path, operation_status]. Each tuple in the vector will 
 *        contain status for the resource value set operation for each resource 
 *        in the input_values vector.  
 * @return MblError returns Error::None if all resources can be accessed according 
 *         to the provided access_token, or error code otherwise.
 */
    MblError set_resource_values(
        const std::string &access_token, 
        const std::vector<MblCloudConnect_ResourcePath_Value> &input_values, 
        std::vector<MblCloudConnect_ResourcePath_Status> &output_set_statuses);

/**
 * @brief Get resource values from multiple resources. 
 *
 * @param access_token token used for access control to the resources pointed 
 *        from input_paths vector. 
 * @param input_paths vector of tuples from the format 
 *        [resource_path, resource_data_type]. Each tuple in vector defines
 *        resource path and data type of the resource value that should be
 *        gotten.
 * @param output_get_values output vector of tuples from the format
 *        [resource_path, resource_typed_data_value, operation_status]. Each 
 *        tuple in the vector will contain output resource value and status of 
 *        the get operation for the resources in the input_paths vector.
 * @return MblError returns Error::None if all resources can be accessed according 
 *         to the provided access_token, or error code otherwise.
 */
    MblError get_resource_values(
        const std::string &access_token, 
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
