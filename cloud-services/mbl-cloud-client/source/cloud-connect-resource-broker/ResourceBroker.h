/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ResourceBroker_h_
#define ResourceBroker_h_

#include "DBusAdapter.h"
#include "RegistrationRecord.h"
#include "CloudConnectTypes.h"

#include <inttypes.h>
#include <memory>
#include <pthread.h>
#include <queue>
#include <atomic>
#include <functional>


class ResourceBrokerTester;
namespace mbl {

/**
 * @brief Class implements functionality of Mbl Cloud Connect Resource 
 * Broker (CCRB). Main functionality: 
 * - receive and manage requests from applications to MbedCloudClient.
 * - send observers notifications from MbedCloudClient to applications.
 */
class ResourceBroker {

friend ::ResourceBrokerTester;
friend DBusAdapterImpl;

public:
    ResourceBroker();
    virtual ~ResourceBroker();

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // API to be used by MblCloudClient class
    ////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Starts CCRB.
     * In details: 
     * - initializes CCRB instance and runs event-loop.
     *
     * @param cloud_client - mbed cloud client
     * @return MblError returns value Error::None if function succeeded, 
     *         or Error::CCRBStartFailed otherwise. 
     */
    virtual MblError start(MbedCloudClient* cloud_client);

    /**
     * @brief Stops CCRB.
     * In details: 
     * - stops CCRB event-loop.
     * - deinitializes CCRB instance.
     * 
     * @return MblError returns value Error::None if function succeeded, 
     *         or Error::CCRBStopFailed otherwise. 
     */
    virtual MblError stop();

protected:

    /**
     * @brief Initializes CCRB instance.
     * 
     * @return MblError returns value Error::None if function succeeded, 
     *         or error code otherwise.
     */
    virtual MblError init();

    /**
     * @brief Deinitializes CCRB instance.
     * 
     * @return MblError returns value Error::None if function succeeded, 
     *         or error code otherwise.
     */
    virtual MblError deinit();

    /**
     * @brief Runs CCRB event-loop.
     * 
     * @return MblError returns value Error::None if function succeeded, 
     *         or error code otherwise.
     */
    virtual MblError run();


    ////////////////////////////////////////////////////////////////////////////////////////////////
    // API to be used by DBusAdapter class
    ////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Starts asynchronous registration request of the resource set 
     * in the Cloud.  
     * This function parses the input json file, and creates resource objects 
     * from it. Created objects pends for the registration to the Cloud. 
     * CCRB will send the final status of the registration to the application 
     * (when it will be ready) by calling update_registration_status API. 
     * 
     * @param ipc_request_handle handle to the IPC unique connection information 
     *        of the application that should get update_registration_status message.
     * @param app_resource_definition_json json file that describes resources 
     *        that should be registered. The structure of the JSON document 
     *        reflects the structure of the required resource tree. 
     * @param out_status cloud connect operation status for operations like 
     *        json file structure validity, sending registration request 
     *        to the Cloud, and so on. 
     *        Note: This parameter is valid, if MblError return error code 
     *        was Error::None.  
     * @param out_access_token is a token that should be used by the client 
     *        application in all APIs that access (in any way) to the provided 
     *        (via app_resource_definition_json) set of resources. 
     *        Note: This parameter is valid, if MblError return error code 
     *        was Error::None.  
     * 
     * @return MblError returns Error::None if resource broker internal operations 
     *         were successfully finished, or error code otherwise. 
     */
    virtual MblError register_resources(
        const uintptr_t ipc_request_handle, 
        const std::string &app_resource_definition_json,
        CloudConnectStatus &out_status,
        std::string &out_access_token);

    /**
     * @brief Starts asynchronous deregistration request of the resource set 
     * from the Cloud.  
     * This function starts deregistration procedure of all resources that are
     * "owned" by access_token. CCRB will send the final status of the 
     * deregistration to the application (when it will be ready) by calling 
     * update_deregistration_status API. 
     * 
     * @param ipc_request_handle handle to the IPC unique connection information 
     *        of the application that should be notified.
     * @param access_token token that defines set of resources that should be 
     *        deregistered.   
     * @param out_status cloud connect operation status for operations like 
     *        access_token validity, sending deregistration request 
     *        to the Cloud, and so on.  
     *        Note: This parameter is valid, if MblError return error code 
     *        was Error::None.  
     * 
     * @return MblError returns Error::None if resource broker internal operations 
     *         were successfully finished, or error code otherwise. 
     */
    virtual MblError deregister_resources(
        const uintptr_t ipc_request_handle, 
        const std::string &access_token,
        CloudConnectStatus &out_status);

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
     * @param out_status cloud connect operation status for operations like 
     *        access_token validity, access permissions to the resources, sending 
     *        add resource instances request to the Cloud, and so on. 
     *        Note: This parameter is valid, if MblError return error code 
     *        was Error::None.  
     * 
     * @return MblError returns Error::None if resource broker internal operations 
     *         were successfully finished, or error code otherwise. 
     */
    virtual MblError add_resource_instances(
        const uintptr_t ipc_request_handle, 
        const std::string &access_token, 
        const std::string &resource_path, 
        const std::vector<uint16_t> &resource_instance_ids,
        CloudConnectStatus &out_status);

    /**
     * @brief Starts resource instances remove asynchronous request 
     * from the Cloud.  
     * This function starts resource instances remove procedure of all 
     * resources instances that are provided in resource_instance_ids. 
     * CCRB will send the final status of the resource instances removal 
     * to the application (when it will be ready) by calling 
     * update_remove_resource_instance_status API.
     * 
     * @param ipc_request_handle handle to the IPC unique connection information 
     *        of the application that should be notified.
     * @param access_token token used for access control to the resource which path 
     *        is provided in resource_path argument.
     * @param resource_path path of the resource from which instances 
     *        should be removed.  
     * @param resource_instance_ids vector of instance ids. Each instance id 
     *        in the vector is an id of the resource instance that should be 
     *        removed from the given resource (identified by resource_path).   
     * @param out_status cloud connect operation status for operations like 
     *        access_token validity, access permissions to the resources, sending 
     *        remove resource instances request to the Cloud, and so on.  
     *        Note: This parameter is valid, if MblError return error code 
     *        was Error::None.  
     * 
     * @return MblError returns Error::None if resource broker internal operations 
     *         were successfully finished, or error code otherwise. 
     */
    virtual MblError remove_resource_instances(
        const uintptr_t ipc_request_handle, 
        const std::string &access_token, 
        const std::string &resource_path, 
        const std::vector<uint16_t> &resource_instance_ids,
        CloudConnectStatus &out_status);

    /**
     * @brief Set resources values for multiple resources. 
     *
     * @param access_token token used for access control to the resources pointed 
     *        from inout_set_operations vector. 
     * @param inout_set_operations vector of structures that provide all input and 
     *        output parameters to perform setting operation. 
     *        Each entry in inout_set_operations contains:
     * 
     *        input fields: 
     *        - input_data is the data that includes resources's path type and value 
     *          of the corresponding resource.
     * 
     *        output field: 
     *        - output_status is the status of the set operation for the corresponding 
     *          resource.
     *        Note: This parameter is valid, if MblError return error code 
     *        was Error::None.  
     *
     * 
     * @param out_status cloud connect operation status for operations like 
     *        access_token validity, access permissions to the resources, and so on. 
     *        The set operation status is not returned via MblError, but by filling 
     *        corresponding value to the output_status in inout_set_operations.
     *        Note: This parameter is valid, if MblError return error code 
     *        was Error::None.  
     * 
     * @return MblError returns Error::None if resource broker internal operations 
     *         were successfully finished, or error code otherwise.
     */
    virtual MblError set_resources_values(
        const std::string &access_token, 
        std::vector<ResourceSetOperation> &inout_set_operations,
        CloudConnectStatus &out_status);

    /**
     * @brief Get resources values from multiple resources. 
     *
     * @param access_token token used for access control to the resources pointed 
     *        from inout_get_operations vector. 
     * @param inout_get_operations vector of structures that provide all input and 
     *        output parameters required to perform getting operation. 
     *        Each entry in inout_get_operations contains:
     * 
     *        input fields: 
     *        - inout_data.path field is the path of the corresponding resource 
     *          who's value should be gotten. 
     *        - inout_data.type field is the type of the resource data.
     * 
     *        output fields: 
     *        - output_status is the status of the set operation for the corresponding
     *          resource.
     *        - inout_data.value field is the value that was gotten from resource. 
     *          Use inout_data.value only if the output_status has SUCCESS value. 
     *        Note: This parameter is valid, if MblError return error code 
     *        was Error::None.  
     *
     * @param out_status cloud connect operation status for operations like 
     *        access_token validity, access permissions to the resources, and so on. 
     *        The get operation status is not returned via MblError, but by filling 
     *        corresponding value to the output_status in inout_get_operations.
     *        Note: This parameter is valid, if MblError return error code 
     *        was Error::None.  
     * 
     * @return MblError returns Error::None if resource broker internal operations 
     *         were successfully finished, or error code otherwise.
     */
    virtual MblError get_resources_values(
        const std::string &access_token, 
        std::vector<ResourceGetOperation> &inout_get_operations,
        CloudConnectStatus &out_status);

    /**
     * @brief CCRB thread main function.
     * In details: 
     * - initializes CCRB module.
     * - runs CCRB main functionality loop.  
     * 
     * @param ccrb address of CCRB instance that should run. 
     * @return void* thread output status. CCRB thread status(MblError enum) returned by value. 
     */
    static void *ccrb_main(void *ccrb);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Callback functions that are being called by Mbed client
    ////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Register callback function that will be called directly by Mbed cloud client
     * 
     */
    void regsiter_callback_handlers();

    /**
     * @brief - Registration update cllback.
     * Called by Mdeb cloud client to indicate last mbed-cloud-client registration update was successful.
     * 
     */
    void handle_registration_updated_cb();

    /**
     * @brief - Error callback function
     * Called by Mdeb cloud client to indicate last mbed-cloud-client operation failure
     * 
     * @param cloud_client_code - Mbed cloud client error code for the last register / deregister operations.
     */
    void handle_error_cb(const int cloud_client_code);

    typedef std::shared_ptr<RegistrationRecord> RegistrationRecord_ptr;
    typedef std::map<std::string, RegistrationRecord_ptr> RegistrationRecordMap;

    /**
     * @brief Generate unique access token using sd_id128_randomize
     * Unique access token is saved in in_progress_access_token_ private member
     * 
     * @return std::pair<MblError, std::string> - a pair where the first element Error::None for 
     * success, therwise the failure reason.
     * If the first element is Error::None, user may access the second element which is the 
     * generate access token. most compilers will optimize (move and not copy). If first element is 
     * not success - user should ignore the second element.
     */
    std::pair<MblError, std::string> generate_access_token();

    /**
     * @brief Return registration record using acceess token
     * 
     * @param access_token - access token arrived from ipc adapter
     * @return Registration record
     */
    ResourceBroker::RegistrationRecord_ptr get_registration_record(const std::string& access_token);

    // Application Enpoints map that holds application that requested to register resoures
    // Used also for other application related operation
    RegistrationRecordMap registration_records_;

    ////////////////////////////////////////////////////////////////////////////////////////////////    

    // No copying or moving 
    // (see https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#cdefop-default-operations)
    ResourceBroker(const ResourceBroker&) = delete;
    ResourceBroker & operator = (const ResourceBroker&) = delete;
    ResourceBroker(ResourceBroker&&) = delete;
    ResourceBroker& operator = (ResourceBroker&&) = delete;

    // thread id of the IPC thread
    pthread_t ipc_thread_id_ = 0;

    // pointer to ipc binder instance
    std::unique_ptr<DBusAdapter> ipc_adapter_ = nullptr;

    MbedCloudClient* cloud_client_;

    // Atomic boolean to signal that rgistration is in progress
    // Use to limit only one allowed registration at a time
    // This member is accessed from CCRB thread and from Mbed client thread (using callbacks)
    std::atomic_bool registration_in_progress_;

    // Access token of the current operation against Mbed client
    std::string in_progress_access_token_;

    // register_update function pointer
    // Mbl cloud client use it to point to Mbed cloud client register_update API
    // Gtests will use it to point to mock function
    std::function<void()> register_update_func_;
    
    // add_objects function pointer
    // Mbl cloud client use it to point to Mbed cloud client add_objects API
    // Gtests will use it to point to mock function
    std::function<void(const M2MObjectList& object_list)> add_objects_func_;
};

} // namespace mbl

#endif // ResourceBroker_h_
