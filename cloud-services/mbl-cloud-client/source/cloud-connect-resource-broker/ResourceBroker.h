/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ResourceBroker_h_
#define ResourceBroker_h_

#include "DBusAdapter.h"
#include "RegistrationRecord.h"
#include "MblMutex.h"
#include "MblScopedLock.h"
#include "update_handlers.h"

#include <pthread.h>
#include <cinttypes>
#include <memory>
#include <semaphore.h>
#include <queue>
#include <atomic>
#include <functional>

extern "C" void resource_broker_shutdown_handler(int signal);

class ResourceBrokerTester;
namespace mbl {

class IpcConnection;
class MailboxMsg;

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

    /**
     * @brief Set a shutdown termination flag
     * Will be handled in main loop
     * Note: This is temporary until we will move signal handling to dbus event loop
     * 
     * @param signal received 
     */
    void resource_broker_shutdown_handler(int signal);


    /**
     * @brief Main loop that create ResourceBroker, initialize and start all components
     * When terminate signal is arrived - stop function will be called, mbed client unregister
     * operation will start, and when unregister callback is called - deinit ResourceBroker.
     * 
     * @return MblError returns value Error::ShutdownRequested if function terminated due to
     *         shutdown signal that was handled correctly, 
     *         or Error::CCRBStartFailed in case start failed.
     */
    static MblError main();

protected:

    ResourceBroker();
    virtual ~ResourceBroker();

    /**
     * @brief Handle mbed client error message
     * When mbed client error callback is called (mbed client thread) - an internal message is sent 
     * to mailbox in order to handle it in internal thread in this function.

     * @param mbed_client_error - mbed client error code
     * @return MblError - MblError::None in case of success or error reason otherwise
     */
    MblError handle_mbed_client_error_internal_message(MblError mbed_client_error);

    /**
     * @brief Handle mbed client registration updated message
     * When mbed client registration updated callback is called (mbed client thread) - an internal 
     * message is sent to mailbox in order to handle it in internal thread in this function
     * 
     * @return MblError - MblError::None in case of success or error reason otherwise
     */
    MblError handle_registration_updated_internal_message();

    /**
     * @brief Process mailbox messages
     * Messages are being sent to mailbox from external thread (e.g. mbed client callbacks), 
     * and processed using this function in internal thread.
     * This will prevent accessing data members from two threads.
     * 
     * @param msg - Mailbox message to proccess
     * @return MblError - Error::None for success running the loop, otherwise the failure reason.
     */
    virtual MblError process_mailbox_message(MailboxMsg& msg);

    /**
     * @brief Starts CCRB.
     * In details: 
     * - initializes CCRB instance and runs event-loop.
     * 
     * Note: This function should be called before ResourceBroker::stop().  
     * 
     * @return MblError returns value Error::None if function succeeded, 
     *         or Error::CCRBStartFailed otherwise. 
     */
    virtual MblError start();

    /**
     * @brief Stops CCRB.
     * In details: 
     * - stops CCRB event-loop.
     * - deinitialize CCRB instance.
     * 
     * Note: This function should be called only after ResourceBroker::start() being called.
     *       This function should be called only from the same thread as ResourceBroker::start() 
     *       was called.
     *
     * @return MblError returns value Error::None if function succeeded, 
     *         or Error::CCRBStopFailed otherwise. 
     */
    virtual MblError stop();

    /**
     * @brief Create mbed client
     * Register the following Mbed cloud client callbacks:
     *    Callback function for successful register device
     *    Callback function for successful un-register device
     *    Callback function for successful registration update (application resources registration)
     *    Callback function for monitoring download progress
     *    Callback function for authorizing firmware downloads and rhandle_authorizeboots
     *    Callback function for occurred error in the above scenariohandle_authorize
     * Start mbed client register operation (register device defaulthandle_authorizeresources)
     * Set internal state to mark that register is in progress
     * 
     * @return MblError - Error::ConnectUnknownError - is case mbed client register operation failed
     *                  - Error::None in case of success
     */
    MblError init_mbed_client();

    /**
     * @brief Deinit mbed client
     * Called after unregister client is finished as part of deinit operation
     * Erase mbed client that was created in init_mbed_client()
     * Stop the mbed event loop thread so no callbacks will be fired
     */
    void deinit_mbed_client();

    /**
     * @brief Unregister mbed client
     * This will set an atomic enum state to signal main loop that unregister is started
     * Unregister operation ends when a callback is called by mbed client.
     * This function is called when terminate signal arrive.
     */
    void unregister_mbed_client();


    /**
     * @brief Initialize CCRB instance.
     * Initialize mbed client using init_mbed_client() API
     * @return MblError returns value Error::None if function succeeded, 
     *         or error code otherwise.
     */
    virtual MblError init();

    /**
     * @brief Deinitialize CCRB instance.
     * Deinitialize mbed client using deinit_mbed_client() API
     * @return MblError returns value Error::None if function succeeded, 
     *         or error code otherwise.
     */
    virtual MblError deinit();

    /**
     * @brief Run CCRB event-loop.
     * 
     * @return MblError returns value Error::None if function succeedhandle_authorizeed, 
     *         or error code otherwise.
     */
    virtual MblError run();


    ////////////////////////////////////////////////////////////////////////////////////////////////
    // API to be used by DBusAdapter class
    ////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Start asynchronous registration request of the resource set 
     * in the Cloud.  
     * This function parses the input json file, and creates resource objects 
     * from it. Created objects pends for the registration to the Cloud. 
     * CCRB will send the final status of the registration to the application 
     * (when it will be ready) by calling update_registration_status API. 
     * 
     * @param source - holds all data related to the to caller IPC connection
     * @param app_resource_definition json file that describes resources 
     *        that should be registered. The structure of the JSON document 
     *        reflects the structure of the required resource tree. 
     * 
     * @return pair - cloud connect operation status for operations (like access_token validity, 
     *                sending deregistration request to the Cloud, and so on)
     *                and access token string which is a token that should be used by the client 
     *                application in all APIs that access to the provided set of resources.
     */
    virtual std::pair<CloudConnectStatus, std::string> 
    register_resources(IpcConnection source, 
                       const std::string &app_resource_definition);

    /**
     * @brief Start asynchronous deregistration request of the resource set 
     * from the Cloud.  
     * This function starts deregistration procedure of all resources that are
     * "owned" by access_token. CCRB will send the final status of the 
     * deregistration to the application (when it will be ready) by calling 
     * update_deregistration_status API. 
     * 
     * @param source - holds all data related to the to caller IPC connection
     * @param access_token token that defines set of resources that should be 
     *        deregistered.   
     * 
     * @return cloud connect operation status for operations like access_token validity, 
     *         sending deregistration request to the Cloud, and so on.
     */
    virtual CloudConnectStatus deregister_resources(
        IpcConnection source, 
        const std::string &access_token);

    /**
     * @brief Start resource instances addition asynchronous request to 
     * the Cloud.  
     * This function starts resource instances addition procedure of all 
     * resources instances that are provided in resource_instance_ids. 
     * CCRB will send the final status of the resource instances addition 
     * to the application (when it will be ready) by calling 
     * update_add_resource_instance_status API.
     * 
     * @param source - holds all data related to the to caller IPC connection
     * @param access_token token used for access control to the resource which path 
     *        is provided in resource_path argument.
     * @param resource_path path of the resource to which instances 
     *        should be added.  
     * @param resource_instance_ids vector of instance ids. Each instance id 
     *        in the vector is an id of the resource instance that should be 
     *        added to the given resource (identified by resource_path).   

     * @return cloud connect operation status for operations like access_token validity, 
     *         sending deregistration request to the Cloud, and so on.
     */
    virtual CloudConnectStatus add_resource_instances(
        IpcConnection source, 
        const std::string &access_token, 
        const std::string &resource_path, 
        const std::vector<uint16_t> &resource_instance_ids);

    /**
     * @brief Start resource instances remove asynchronous request 
     * from the Cloud.  
     * This function starts resource instances remove procedure of all 
     * resources instances that are provided in resource_instance_ids. 
     * CCRB will send the final status of the resource instances removal 
     * to the application (when it will be ready) by calling 
     * update_remove_resource_instance_status API.
     * 
     * @param source - holds all data related to the to caller IPC connection
     * @param access_token token used for access control to the resource which path 
     *        is provided in resource_path argument.
     * @param resource_path path of the resource from which instances 
     *        should be removed.  
     * @param resource_instance_ids vector of instance ids. Each instance id 
     *        in the vector is an id of the resource instance that should be 
     *        removed from the given resource (identified by resource_path).
     * 
     * @return cloud connect operation status for operations like access_token validity, 
     *         sending deregistration request to the Cloud, and so on.
     */
    virtual CloudConnectStatus remove_resource_instances(
        IpcConnection source, 
        const std::string &access_token, 
        const std::string &resource_path, 
        const std::vector<uint16_t> &resource_instance_ids);

    /**
     * @brief Set resources values for multiple resources. 
     *  
     * @param source - holds all data related to the to caller IPC connection
     * 
     * @param access_token token used for access control to the resources pointed 
     *        from inout_set_operations vector. 
     * @param inout_set_operations vector of structures that provide all input and 
     *        output parameters to perform setting operation. 
     *        Each entry in inout_set_operations contains:
     * handle_authorize
     *        input fields: 
     *        - input_data is the data that includes resources's path type and value 
     *          of the corresponding resource.
     * 
     *        output field: 
     *        - output_status is the status of the set operation for the corresponding 
     *          resource.
     *        Note: This parameter is valid, if CloudConnectStatus return error code 
     *        is CloudConnectStatus::STATUS_SUCCESS.
     *
     * @return cloud connect operation status for operations like access_token validity, 
     *         sending deregistration request to the Cloud, and so on.
     */
    virtual CloudConnectStatus set_resources_values(
        IpcConnection source,
        const std::string &access_token, 
        std::vector<ResourceSetOperation> &inout_set_operations);

    /**
     * @brief Get resources values from multiple resources. 
     *
     * @param source - holds all data related to the to caller IPC connection
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
     *        Note: This parameter is valid, if CloudConnectStatus return error code 
     *        is CloudConnectStatus::STATUS_SUCCESS. 
     *
     * @return cloud connect operation status for operations like access_token validity, 
     *         sending deregistration request to the Cloud, and so on.
     */
    virtual CloudConnectStatus get_resources_values(
        IpcConnection source,
        const std::string &access_token, 
        std::vector<ResourceGetOperation> &inout_get_operations);

    /**
     * @brief Inform CCRB that a connection has been closed
     * This is a one-side notification. Nothing is expected to be returned. 
     * (No operation is requested by caller so the internal outcomes are irrelevant for the caller)
     * 
     * @param source - connection which has been closed
     */    
    virtual void notify_connection_closed(IpcConnection source);

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
     * @brief Device registered callback.
     * Called by Mbed cloud client to indicate that device is registered.
     */
    void handle_mbed_client_registered();

    /**
     * @brief Device unregistered callback.
     * Called by Mbed cloud client to indicate that device is no longer registered.
     */
    void handle_mbed_client_unregistered();

    /**
     * @brief Callback function for authorizing firmware downloads and reboots.
     * Called by mbed client
     * 
     * @param request - Request being authorized (enum defined in MbedCloudClient.h):
     *                  UpdateClient::RequestInvalid
     *                  UpdateClient::RequestDownload
     *                  UpdateClient::RequestInstall
     */
    static void handle_mbed_client_authorize(int32_t request);

    /**
     * @brief Keepalive periodic event data
     * 
     */
    struct EventData_Keepalive
    {
        ResourceBroker* ccrb_this;
    };

    /**
     * @brief Periodic keepalive callback to notify Mbed cloud client that device is alive
     * 
     * @param s - event source
     * @param ev - event data
     * @return Error::None in case of success
     */
    static MblError periodic_keepalive_callback(sd_event_source* s, const Event* ev);

    /**
     * @brief - Registration update callback.
     * Called by Mbed cloud client to indicate last mbed-cloud-client registration update was successful.
     * 
     */
    void handle_mbed_client_registration_updated();

    /**
     * @brief - Error callback function
     * Called by Mbed Cloud Client to indicate last mbed-cloud-client operation failure
     * 
     * @param cloud_client_code - Mbed cloud client error code for the last register / deregister operations.
     */
    void handle_mbed_client_error(const int cloud_client_code);

    typedef std::shared_ptr<RegistrationRecord> RegistrationRecord_ptr;
    typedef std::map<std::string, RegistrationRecord_ptr> RegistrationRecordMap;

    /**
     * @brief Validate resource data to be used in set/get operation against the registered resource
     * 
     * @param registration_record - Registration record
     * @param resource_data - Resource data to be used in set / get operation
     * 
     * @return CloudConnectStatus - 
     *      CloudConnectStatus::ERR_INVALID_RESOURCE_PATH - In case of invalid resource path
     *      CloudConnectStatus::ERR_RESOURCE_NOT_FOUND - In case resource not found
     *      CloudConnectStatus::ERR_INVALID_RESOURCE_TYPE - In case of invalid resource type
     *      CloudConnectStatus::STATUS_SUCCESS - In case of valid resource data
     */
    CloudConnectStatus validate_resource_data(const RegistrationRecord_ptr registration_record,
                                              const ResourceData& resource_data);

    /**
     * @brief Validate set operation param and update each resource output status.
     * Used by set_resources_values API.
     * 
     * @param registration_record - Registration record for set operation
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
     * @return bool - true if inout_set_operations is valid, false if not.
     */
    bool validate_set_resources_input_params(
        const RegistrationRecord_ptr registration_record,
        std::vector<ResourceSetOperation>& inout_set_operations);

    /**
     * @brief Validate get operation param and update each resource output status.
     * Used by get_resources_values API.
     * 
     * @param registration_record - Registration record for set operation
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
     * @return bool - true if inout_get_operations is valid, false if not.
     */
    bool validate_get_resources_input_params(
        RegistrationRecord_ptr registration_record,
        std::vector<ResourceGetOperation>& inout_get_operations);

    /**
     * @brief Set value of one resource
     * Used by set_resources_values API
     * 
     * @param registration_record - Registration record
     * @param resource_data - Resource data to be used in set operation
     * 
     * @return CloudConnectStatus - 
     *      CloudConnectStatus::ERR_INTERNAL_ERROR - In case set value in Mbed client failed
     *      CloudConnectStatus::STATUS_SUCCESS - In case of successful set resource data operation
     */
    CloudConnectStatus set_resource_value(const RegistrationRecord_ptr registration_record,
                                          const ResourceData resource_data);

    /**
     * @brief Get value of one resource
     * Used by get_resources_values API
     * 
     * @param registration_record - Registration record
     * @param resource_data - Resource data to be used in get operation
     */
    void get_resource_value(const RegistrationRecord_ptr registration_record,
                            ResourceData& resource_data);

    /**
     * @brief Set mbed cloud client function pointers.
     * In case of tests we will use it to set mbed cloud client function pointers to mock class.
     */
    void init_mbed_client_function_pointers();

    /**
     * @brief Return registration record using access token
     * 
     * @param access_token - access token arrived from ipc adapter
     * @return Registration record
     */
    ResourceBroker::RegistrationRecord_ptr get_registration_record(const std::string& access_token);

    /**
     * @brief Get the registration record of an application that is registering its resources.
     * Only one application is allowed to register its resources at a time.
     * 
     * @return RegistrationRecord - if register of application resources is in progress
     *         or nullptr in case no application is in progress of registering its resources.
     */
    ResourceBroker::RegistrationRecord_ptr get_registration_record_register_in_progress();

    /**
     * @brief Check if register of application resources is in progress
     * 
     * @return true - in case register of application resources is in progress
     * @return false - in case register of application resources is not in progress.
     */
    bool is_registration_record_register_in_progress();

    // Registration record map that holds records of registered / register requests
    // Used also for other application related operation
    // Modifing this map should be done carefully as it can be done in callbacks and in Broker's
    // register_resource API.
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

    // FIXME: init_sem_ and init_sem_initialized_ should be removed in IOTMBL-1707.      
    // semaphore for the initialization procedure syncronization.
    sem_t init_sem_;
    // flag that marks if the init_sem_ was intialized and requires destroy.
    std::atomic_bool init_sem_initialized_;

    // pointer to ipc binder instance
    std::unique_ptr<DBusAdapter> ipc_adapter_;

    // Mbed cloud client
    MbedCloudClient* cloud_client_;
    
    // pointer to a static ResourceBroker instance to be used by callbacks
    // initialized in ResourceBroker c'tor
    static ResourceBroker* s_ccrb_instance;

    // Dummy network interface needed by cloud_client_->setup API (used only in MbedOS)
    static uint32_t dummy_network_interface_;

    // Mbed client states
    enum MbedClientState
    {
        State_ClientUnregisterInProgress,
        State_ClientUnregistered,
        State_ClientRegisterInProgress,
        State_ClientRegistered
    };

    /**
     * @brief Atomic enum to signal which state mbed_client is in
     * This member is accessed from CCRB thread and from Mbed client thread (using callbacks)
     */
    std::atomic<MbedClientState> mbed_client_state_;

    // Access token of the current operation against Mbed client
    std::string reg_update_in_progress_access_token_;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Mbed client function pointers
    ////////////////////////////////////////////////////////////////////////////////////////////////

    // This function pointer will be used in init() to initialize mbed cloud client.
    // In case of tests we will use it to call mock function
    std::function<MblError()> init_mbed_client_func_;

    std::function<void()> deinit_mbed_client_func_;

    // register_update function pointer
    // Mbl cloud client use it to point to Mbed cloud client register_update API
    // Gtests will use it to point to mock function
    std::function<void()> mbed_client_register_update_func_;
    
    // add_objects function pointer
    // Mbl cloud client use it to point to Mbed cloud client add_objects API
    // Gtests will use it to point to mock function
    std::function<void(const M2MObjectList& object_list)> mbed_client_add_objects_func_;

    std::function<const ConnectorClientEndpointInfo*()> mbed_client_endpoint_info_func_;
    std::function<const char *()> mbed_client_error_description_func_;
    ////////////////////////////////////////////////////////////////////////////////////////////////    
};

} // namespace mbl

#endif // ResourceBroker_h_
