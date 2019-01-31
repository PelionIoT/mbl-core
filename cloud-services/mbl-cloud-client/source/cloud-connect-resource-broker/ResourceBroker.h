/*
 * Copyright (c) 2016-2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: ...
 */

#ifndef ResourceBroker_h_
#define ResourceBroker_h_

#include "DBusAdapter.h"
#include "CloudConnectTypes.h"

#include  <memory>


namespace mbl {

/**
 * @brief Class implements functionality of Mbl Cloud Connect Resource 
 * Broker (CCRB). Main functionality: 
 * - receive and manage requests from applications to MbedCloudClient.
 * - send observers notifications from MbedCloudClient to applications.
 */
class ResourceBroker {
friend DBusAdapter;
public:
    ResourceBroker();
    ~ResourceBroker();

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


/////////////////////////////////////////////////////////////////////
// API to be used by DBusAdapter class 
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
 * @brief Set resources values for multiple resources. 
 *
 * @param access_token token used for access control to the resources pointed 
 *        from input_values vector. 
 * @param inout_data vector of entries that provides input and output parameters
 *        for set operation. 
 *        input  params: resource_data_
 *        output params: set_operation_status_
 * @return MblError returns Error::None if all resources can be accessed according 
 *         to the provided access_token, or error code otherwise.
 */
    MblError set_resources_values(
        const std::string &access_token, 
        std::vector<ResourceSetOperation> &inout_get_operations);

/**
 * @brief Get resources values from multiple resources. 
 *
 * @param access_token token used for access control to the resources pointed 
 *        from input_paths vector. 
 * @param inout_data vector of entries that provides input and output parameters
 *        for set operation. 
 *        input  params: resource_data_.path and resource_data_.type 
 *        output params: resource_data_.value and get_operation_status
 * @return MblError returns Error::None if all resources can be accessed according 
 *         to the provided access_token, or error code otherwise.
 */
    MblError get_resources_values(
        const std::string &access_token, 
        std::vector<ResourceGetOperation> &inout_set_operations);

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


// FIX return MblError description !!!!!!!!!!!!!!!!!!!!!!!!!
// FIXME ResourceBroker

private:

    // No copying or moving (see https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#cdefop-default-operations)
    ResourceBroker(const ResourceBroker&) = delete;
    ResourceBroker & operator = (const ResourceBroker&) = delete;
    ResourceBroker(ResourceBroker&&) = delete;
    ResourceBroker& operator = (ResourceBroker&&) = delete;  

    // thread id of the IPC thread
    pthread_t ipc_thread_id_ = 0;

    // pointer to ipc binder instance
    std::unique_ptr<DBusAdapter> ipc_ = nullptr;
};

} // namespace mbl

#endif // ResourceBroker_h_
