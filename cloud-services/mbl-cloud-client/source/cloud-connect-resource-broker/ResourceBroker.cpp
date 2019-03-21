/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ResourceBroker.h"
#include "CloudConnectTrace.h"
#include "CloudConnectTypes.h"
#include "mbed-cloud-client/MbedCloudClient.h"
#include "mbed_cloud_client_user_config.h"
#include "ns-hal-pal/ns_event_loop.h"

#include <cassert>
#include <csignal>
#include <time.h>
#include <unistd.h>

#define TRACE_GROUP "ccrb"

static volatile sig_atomic_t g_shutdown_signal_once = 0;

// Period between re-registrations with the LWM2M server.
// MBED_CLOUD_CLIENT_LIFETIME (seconds) is how long we should stay registered after each
// re-registration (keepalive)
static const int g_keepalive_period_miliseconds = (MBED_CLOUD_CLIENT_LIFETIME / 2) * 1000;

// This is temporary until we will move signal handling to dbus event loop
extern "C" void resource_broker_shutdown_handler(int signal)
{
    g_shutdown_signal_once = signal;
}

namespace mbl
{

ResourceBroker* ResourceBroker::s_ccrb_instance = nullptr;
uint32_t ResourceBroker::dummy_network_interface_ = 0xFFFFFFFF;

MblError ResourceBroker::main()
{
    TR_DEBUG_ENTER;
    ResourceBroker resource_broker;
    assert(s_ccrb_instance);

    // Note: start() will init mbed client and state_ is moved to State_ClientRegisterInProgress.
    const MblError ccrb_start_err = resource_broker.start();
    if (Error::None != ccrb_start_err) {
        TR_ERR("CCRB module start() failed! (%s)", MblError_to_str(ccrb_start_err));
        return ccrb_start_err;
    }
    TR_INFO("ResourceBroker started successfully");

    for (;;) {

        if (0 != g_shutdown_signal_once) {

            TR_WARN("Received signal: %s, Un-registering device...",
                    strsignal(g_shutdown_signal_once));
            resource_broker.unregister_mbed_client();
            g_shutdown_signal_once = 0;
        }

        if (State_ClientUnregistered == resource_broker.mbed_client_state_.load()) {

            TR_DEBUG("State is unregistered - stop ccrb_main thread");
            // Unregister device is finished - stop resource broker
            // This will close ccrb_main thread, and deinit will be called
            const MblError ccrb_stop_err = resource_broker.stop();
            if (Error::None != ccrb_stop_err) {
                TR_ERR("CCRB module stop() failed! (%s)", MblError_to_str(ccrb_stop_err));
                return ccrb_stop_err;
            }

            return Error::ShutdownRequested;
        }

        sleep(1);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// This constructor is called from main function
ResourceBroker::ResourceBroker()
    : init_sem_initialized_(false),
      ipc_adapter_(nullptr),
      cloud_client_(nullptr),
      mbed_client_state_(State_ClientUnregistered)
{
    TR_DEBUG_ENTER;

    s_ccrb_instance = this;

    // This function pointers will be used in init() / deinit() of mbed cloud client
    // In case of tests we will use it call mock function
    init_mbed_client_func_ = std::bind(
        static_cast<MblError (ResourceBroker::*)()>(&ResourceBroker::init_mbed_client), this);
    deinit_mbed_client_func_ = std::bind(&ResourceBroker::deinit_mbed_client, this);
}

ResourceBroker::~ResourceBroker()
{
    TR_DEBUG_ENTER;
    s_ccrb_instance = nullptr;
}

void ResourceBroker::unregister_mbed_client()
{
    TR_DEBUG_ENTER;

    mbed_client_state_.store(State_ClientUnregisterInProgress);

    TR_INFO("Close mbed client");

    cloud_client_->close();
}

MblError ResourceBroker::start()
{
    // FIXME: This function should be refactored  in IOTMBL-1707.
    // Initialization of the semaphore and call to the sem_timedwait will be removed.

    TR_DEBUG_ENTER;

    // initialize init semaphore
    int ret = sem_init(&init_sem_,
                       0 /* semaphore is used between threads in the process */,
                       0 /* create "locked" semaphore, with initial value 0 */);
    if (0 != ret) {
        // semaphore init failed, print errno value and exit
        TR_ERRNO("sem_init", errno);
        return Error::SystemCallFailed;
    }

    // mark that semaphore was initialized successfully, so we must call sem_destroy
    init_sem_initialized_.store(true);

    // create new thread which will run IPC event loop
    ret = pthread_create(&ipc_thread_id_,
                         nullptr, // thread is created with default attributes
                         ResourceBroker::ccrb_main,
                         this);
    if (0 != ret) {
        // thread creation failed, print errno value and exit
        TR_ERRNO("pthread_create", errno);
        return Error::SystemCallFailed;
    }

    struct timespec timeout_time = {0, 0};
    if (0 != clock_gettime(CLOCK_REALTIME, &timeout_time)) {
        TR_ERRNO("clock_gettime", errno);
        return Error::SystemCallFailed;
    }

    // set initialization procedure timeout of 2 secs
    timeout_time.tv_sec += 2;

    // wait for initialization procedure to finish
    while ((0 != (ret = sem_timedwait(&init_sem_, &timeout_time))) && (errno == EINTR)) {
        continue; // restart if interrupted by signal handler
    }

    // check what happened
    if (0 != ret) {
        // analyse why sem_timedwait failed
        if (errno == ETIMEDOUT) {
            // the timeout ocurres when by some reason initialization procedure was not finished
            // during expected period of time(much less than 2 seconds). Possible reasons can be
            // that CCRB thread exited with error, or initialization procedure failed.
            TR_ERRNO_F("sem_timedwait", ETIMEDOUT, "Timeout ocurred!");
            return Error::IpcTimeout;
        }
        TR_ERRNO("sem_timedwait", errno);
        return Error::SystemCallFailed;
    }
    // init_sem_ was signalled
    TR_INFO("Resource Broker initializations finished successfully");

    // thread was created
    return Error::None;
}

MblError ResourceBroker::stop()
{
    TR_DEBUG_ENTER;

    // FIXME: handle properly all errors in this function.

    assert(ipc_adapter_);

    // try sending stop signal to ipc (pass no error)
    const MblError ipc_stop_err = ipc_adapter_->stop(MblError::None);
    if (Error::None != ipc_stop_err) {
        TR_ERR("ipc::stop failed! (%s)", MblError_to_str(ipc_stop_err));

        // FIXME: Currently, if ipc was not successfully signalled, we return error.
        //        Required to add "release resources best effort" functionality.
        return ipc_stop_err;
    }

    // about thread_status: thread_status is the variable that will contain ccrb_main status.
    // ccrb_main returns MblError type. pthread_join will assign the value returned via
    // pthread_exit to the value of pointer thread_status. For example, if ccrb_main
    // will return N via pthread_exit(N), value of pointer thread_status will be equal N.
    void* thread_status = nullptr; // do not dereference the pointer!

    // ipc was succesfully signalled to stop, join with thread.
    const int thread_join_err = pthread_join(ipc_thread_id_, &thread_status);
    // FIXME: Currently, we use pthread_join without timeout.
    //        Required to use pthread_join with timeout.
    if (0 != thread_join_err) {
        // thread joining failed, print errno value
        TR_ERR("Thread joining failed (%s)", strerror(errno));

        // FIXME: Currently, if pthread_join fails, we return error.
        //        Required to add "release resources best effort" functionality.
        return Error::IpcProcedureFailed;
    }

    // thread was succesfully joined, handle thread_status. Thread_status will contain
    // value that was returned via pthread_Exit(MblError) from ccrb main function.
    uintptr_t thread_ret_value = reinterpret_cast<uintptr_t>(thread_status); // NOLINT
    OneSetMblError ret_value(static_cast<MblError>(thread_ret_value));
    if (Error::None != ret_value.get()) {
        // the return with an error ret_value indicate of the reason for exit, and the
        // failure of the current function.
        TR_ERR("ccrb_main() exit with error %s", ret_value.get_status_str());
    }

    if (init_sem_initialized_.load()) {
        // destroy semaphore required
        const int sem_destroy_err = sem_destroy(&init_sem_);
        if (0 != sem_destroy_err) {
            // semaphore destroy failed, print errno value
            TR_ERRNO("sem_destroy", errno);
            ret_value.set(Error::SystemCallFailed);
        }

        // destroy was performed, so semaphore uninitialized
        init_sem_initialized_.store(false);
    }

    return ret_value.get();
}

MblError ResourceBroker::periodic_keepalive_callback(sd_event_source* s, const Event* ev)
{
    TR_DEBUG_ENTER;

    UNUSED(s);
    assert(ev);

    TR_DEBUG("%s event", ev->get_description().c_str());

    ResourceBroker* const this_ccrb =
        static_cast<ResourceBroker*>(ev->get_data().user_data); // NOLINT

    MbedClientState current_state = this_ccrb->mbed_client_state_.load();
    // Check if application registration update is in progress
    // if yes - do nothing as registration will act as keep alive
    if (this_ccrb->is_registration_record_register_in_progress()) {
        TR_DEBUG("Application registration update is in progress- no need for keepalive.");
        return Error::None;
    }

    // Keep alive is only needed when device is registered
    if (State_ClientRegistered != current_state) {
        TR_DEBUG("Device is not registered.");
        return Error::None;
    }

    TR_DEBUG("Call cloud_client_->register_update");
    this_ccrb->mbed_client_register_update_func_();

    return Error::None;
}

void ResourceBroker::init_mbed_client_function_pointers()
{
    TR_DEBUG_ENTER;

    assert(nullptr != cloud_client_);

    mbed_client_add_objects_func_ = std::bind(
        static_cast<void (MbedCloudClient::*)(const M2MObjectList&)>(&MbedCloudClient::add_objects),
        cloud_client_,
        std::placeholders::_1);

    mbed_client_register_update_func_ = std::bind(&MbedCloudClient::register_update, cloud_client_);

    mbed_client_endpoint_info_func_ =
        std::bind(static_cast<const ConnectorClientEndpointInfo* (MbedCloudClient::*) () const>(
                  &MbedCloudClient::endpoint_info),
                  cloud_client_);

    mbed_client_error_description_func_ = std::bind(
        static_cast<const char* (MbedCloudClient::*) () const>(&MbedCloudClient::error_description),
        cloud_client_);
}

MblError ResourceBroker::init_mbed_client()
{
    TR_DEBUG_ENTER;

    assert(nullptr == cloud_client_);

    cloud_client_ = new MbedCloudClient();

    // 1. Set function pointers to point to mbed_client functions
    //    (In gtest our friend test class will override these pointers to get all the calls)
    init_mbed_client_function_pointers();

    // 2. Register mbed client callback:
    cloud_client_->on_registered(this, &ResourceBroker::handle_mbed_client_registered);
    cloud_client_->on_unregistered(this, &ResourceBroker::handle_mbed_client_unregistered);
    cloud_client_->on_registration_updated(
        this, &ResourceBroker::handle_mbed_client_registration_updated);
    cloud_client_->set_update_progress_handler(&update_handlers::handle_download_progress);
    cloud_client_->set_update_authorize_handler(&handle_mbed_client_authorize);
    cloud_client_->on_error(this, &ResourceBroker::handle_mbed_client_error);

    // 3. Register Device Default Resources
    M2MObjectList objs; // Using empty ObjectList
    mbed_client_add_objects_func_(objs);

    // Dummy network interface needed by cloud_client_->setup API (used only in MbedOS)
    const bool setup_ok = cloud_client_->setup((void*) &dummy_network_interface_);
    if (!setup_ok) {
        TR_ERR("Mbed cloud client setup failed");
        return Error::ConnectUnknownError; // state_ will stay on State_ClientUnregistered
    }

    mbed_client_state_.store(State_ClientRegisterInProgress);

    return Error::None;
}

MblError ResourceBroker::init()
{
    TR_DEBUG_ENTER;

    // verify that ipc_adapter_ member was not created yet
    assert(nullptr == ipc_adapter_);

    // create ipc instance and pass ccrb instance to constructor
    ipc_adapter_ = std::make_unique<DBusAdapter>(*this);

    const MblError ipc_adapter_init_status = ipc_adapter_->init();
    if (Error::None != ipc_adapter_init_status) {
        TR_ERR("ipc::init failed with error %s", MblError_to_str(ipc_adapter_init_status));
        return ipc_adapter_init_status;
    }

    // Init Mbed cloud client
    const MblError init_mbed_clinet_status = init_mbed_client_func_();
    if (Error::None != init_mbed_clinet_status) {
        TR_ERR("init_mbed_client failed with error %s", MblError_to_str(init_mbed_clinet_status));
        return init_mbed_clinet_status;
    }

    // Set keepalive periodic event
    Event::EventData event_data = {0};
    event_data.user_data = this; // NOLINT
    std::pair<MblError, uint64_t> ret_pair =
        ipc_adapter_->send_event_periodic(event_data,
                                          sizeof(event_data.user_data), // NOLINT
                                          Event::EventDataType::USER_DATA_TYPE,
                                          ResourceBroker::periodic_keepalive_callback,
                                          g_keepalive_period_miliseconds,
                                          "Mbed cloud client keep-alive");

    if (Error::None != ret_pair.first) {
        TR_ERR("send_event_periodic keep-alive failed with error %s",
               MblError_to_str(ret_pair.first));
        return ret_pair.first;
    }

    return Error::None;
}

void ResourceBroker::deinit_mbed_client()
{
    TR_DEBUG_ENTER;

    assert(cloud_client_);
    TR_INFO("Erase mbed client");    
    delete cloud_client_;
    TR_INFO("Stop the mbed event loop thread");
    ns_event_loop_thread_stop();
}

MblError ResourceBroker::deinit()
{
    TR_DEBUG_ENTER;

    assert(ipc_adapter_);

    // FIXME: Currently we call ipc_adapter_->deinit unconditionally.
    //        ipc_adapter_->deinit can't be called if ccrb thread was not finished.
    //        Required to call ipc_adapter_->deinit only if ccrb thread was finished.
    MblError status = ipc_adapter_->deinit();
    if (Error::None != status) {
        TR_ERR("ipc::deinit failed with error %s", MblError_to_str(status));
    }

    ipc_adapter_.reset(nullptr);

    // This is done after device is unregistered (no mbed client callbacks will arrive)
    deinit_mbed_client_func_();

    return status;
}

MblError ResourceBroker::run()
{
    TR_DEBUG_ENTER;
    MblError stop_status;

    assert(ipc_adapter_);

    MblError status = ipc_adapter_->run(stop_status);
    if (Error::None != status) {
        TR_ERR("ipc::run failed with error %s", MblError_to_str(status));
        return status;
    }
    TR_DEBUG("ipc::run successfully stopped");
    return status;
}

void* ResourceBroker::ccrb_main(void* ccrb)
{
    TR_DEBUG_ENTER;

    assert(ccrb);

    ResourceBroker* const this_ccrb = static_cast<ResourceBroker*>(ccrb);

    OneSetMblError status(this_ccrb->init());
    if (Error::None != status.get()) {
        // Not returning as we want to call deinit below
        TR_ERR("CCRB::init failed with error %s. Exit CCRB thread.", status.get_status_str());
    }

    // Signal to the semaphore that initialization was finished
    const int ret = sem_post(&this_ccrb->init_sem_);
    if (0 != ret) {
        TR_ERRNO("semaphore post failed ", errno);
        status.set(Error::IpcProcedureFailed); // continue to deinit and return status
    }

    // Call ccrb->run only if init succeeded
    if (Error::None == status.get()) {
        const MblError run_status = this_ccrb->run();
        if (Error::None != run_status) {
            TR_ERR("CCRB::run failed with error %s. Exit CCRB thread.",
                   MblError_to_str(run_status));
            status.set(run_status); // continue to deinit and return status
        }
    }

    const MblError deinit_status = this_ccrb->deinit();
    if (Error::None != deinit_status) {
        TR_ERR("CCRB::deinit failed with error %s. Exit CCRB thread.",
               MblError_to_str(deinit_status));
        status.set(deinit_status);
    }

    TR_INFO("CCRB thread function finished with status: %s", status.get_status_str());
    uintptr_t final_status = static_cast<uintptr_t>(status.get());
    pthread_exit(reinterpret_cast<void*>(final_status)); // NOLINT
}

ResourceBroker::RegistrationRecord_ptr
ResourceBroker::get_registration_record(const std::string& access_token)
{
    TR_DEBUG_ENTER;

    auto itr = registration_records_.find(access_token);
    if (registration_records_.end() == itr) {
        // Could not found registration_record
        TR_ERR("Registration record (access_token: %s) does not exist.", access_token.c_str());
        return nullptr;
    }

    return itr->second;
}

ResourceBroker::RegistrationRecord_ptr
ResourceBroker::get_registration_record_register_in_progress()
{
    TR_DEBUG_ENTER;

    auto itr = registration_records_.begin();
    while (itr != registration_records_.end()) {
        if (RegistrationRecord::State_RegistrationInProgress == 
             itr->second->get_registration_state()) {
            return itr->second;
        }
        itr++;
    }

    return nullptr;
}

bool ResourceBroker::is_registration_record_register_in_progress()
{
    TR_DEBUG_ENTER;

    auto itr = registration_records_.begin();
    while (itr != registration_records_.end()) {
        if (RegistrationRecord::State_RegistrationInProgress == 
             itr->second->get_registration_state()) {
            return true;
        }
        itr++;
    }

    return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// Callback functions that are being called by Mbed client
////////////////////////////////////////////////////////////////////////////////////////////////////

void ResourceBroker::handle_mbed_client_registered()
{
    TR_DEBUG_ENTER;

    // In case terminate signal was received during register request - ignore registration flow
    // and continue un-registering device
    if (State_ClientUnregisterInProgress == mbed_client_state_.load()) {
        TR_WARN("client_registered callback was called while trying to un-register.");
        return;
    }

    mbed_client_state_.store(State_ClientRegistered);

    TR_INFO("Client registered");

    const ConnectorClientEndpointInfo* endpoint = mbed_client_endpoint_info_func_();
    if (nullptr == endpoint) {
        TR_WARN("Failed to get endpoint info");
        return;
    }

    TR_INFO("Endpoint Name: %s", endpoint->endpoint_name.c_str());
    TR_INFO("Device Id: %s", endpoint->internal_endpoint_name.c_str());
}

void ResourceBroker::handle_mbed_client_unregistered()
{
    TR_DEBUG_ENTER;

    mbed_client_state_.store(State_ClientUnregistered);
    TR_WARN("Client unregistered");
}

void ResourceBroker::handle_mbed_client_authorize(const int32_t request)
{
    TR_DEBUG_ENTER;

    if (update_handlers::handle_authorize(request)) {
        if (nullptr != s_ccrb_instance) {
            s_ccrb_instance->cloud_client_->update_authorize(request);
        }
    }
}

void ResourceBroker::handle_mbed_client_registration_updated()
{
    TR_DEBUG_ENTER;

    // TODO: need to handle cases when mbed client does not call any callback
    // during registration - IOTMBL-1700

    // This function can be called as a result of:
    // 1. application requested to register resources
    // 2. keep alive (does not occur during #1)
    
    // Only one application can be in registration update state - check if there is such...
    RegistrationRecord_ptr registration_record = get_registration_record_register_in_progress();
    if (nullptr != registration_record) {

        TR_DEBUG("Registration record (access_token: %s) registered successfully.",
                 reg_update_in_progress_access_token_.c_str());

        registration_record->set_registration_state(
            RegistrationRecord::State_Registered);

        // Send the response to adapter:
        CloudConnectStatus reg_status = CloudConnectStatus::STATUS_SUCCESS;
        const MblError status = ipc_adapter_->update_registration_status(
            registration_record->get_registration_source(), reg_status);
        if (Error::None != status) {
            TR_ERR("update_registration_status failed with error: %s", MblError_to_str(status));
        }
        reg_update_in_progress_access_token_.clear();
        return;
    }

    TR_DEBUG("Keepalive finished successfully");
}

// TODO: this callback is related to the following scenarios which are not yet implemented:
// add resource instances and remove resource instances.
void ResourceBroker::handle_mbed_client_error(const int cloud_client_code)
{
    TR_DEBUG_ENTER;

    const MblError mbl_code =
        CloudClientError_to_MblError(static_cast<MbedCloudClient::Error>(cloud_client_code));
    TR_ERR("Error occurred: %d: %s", mbl_code, MblError_to_str(mbl_code));
    TR_ERR("Error details: %s", mbed_client_error_description_func_());

    MbedClientState current_state = mbed_client_state_.load();

    // Client register in progress:
    if (State_ClientRegisterInProgress == current_state) { 
        TR_ERR("Client register failed.");
        // We have no choice but to signal that device is unregistered which will cause to
        // closeing mbed client and unregister ungracefully
        mbed_client_state_.store(State_ClientUnregistered);
        return;
    }

    // Client unregister in progress:
    if (State_ClientUnregisterInProgress == current_state) {
        TR_ERR("Client unregister failed.");
        // We have no choice but to signal that device is unregistered which will cause to
        // closeing mbed client and unregister ungracefully
        mbed_client_state_.store(State_ClientUnregistered);
        return;
    }

    // Check if this callback is caused by an application trying to register resources
    // (only one application can be in registration update state at a time)
    RegistrationRecord_ptr registration_record = get_registration_record_register_in_progress();
    if (nullptr != registration_record) {

        TR_ERR("Registration (access_token: %s) failed.",
               reg_update_in_progress_access_token_.c_str());

        const MblError status = ipc_adapter_->update_registration_status(
            registration_record->get_registration_source(), CloudConnectStatus::ERR_INTERNAL_ERROR);
        if (Error::None != status) {
            TR_ERR("ipc_adapter_->update_registration_status failed with error: %s",
                   MblError_to_str(status));
        }

        TR_DEBUG("Erase registration record (access_token: %s)",
                 reg_update_in_progress_access_token_.c_str());

        auto itr = registration_records_.find(reg_update_in_progress_access_token_);
        registration_records_.erase(itr); // Erase registration record as regitsration failed
        reg_update_in_progress_access_token_.clear();
        return;
    }

    // Keepalive:
    if (State_ClientRegistered == current_state) {
        TR_ERR("Keepalive request failed.");
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

std::pair<CloudConnectStatus, std::string>
ResourceBroker::register_resources(IpcConnection source, const std::string& app_resource_definition)
{
    TR_DEBUG_ENTER;

    std::pair<CloudConnectStatus, std::string> ret_pair(CloudConnectStatus::STATUS_SUCCESS,
                                                        std::string());

    if (State_ClientRegistered != mbed_client_state_.load()) {
        TR_ERR("Client is not registered.");
        ret_pair.first = CloudConnectStatus::ERR_INTERNAL_ERROR;
        return ret_pair;
    }
    
    // Only one register update request is allowed at a time:
    if (is_registration_record_register_in_progress()) {
        TR_ERR("Registration is already in progress.");
        ret_pair.first = CloudConnectStatus::ERR_REGISTRATION_ALREADY_IN_PROGRESS;
        return ret_pair;
    }

    // Above check makes sure there is no registration in progress.
    // Lets check if an app is already not registered...
    // TODO: remove this check when supporting multiple applications
    if (!registration_records_.empty()) {
        // Currently support only ONE application
        TR_ERR("Only one registration is allowed.");
        ret_pair.first = CloudConnectStatus::ERR_ALREADY_REGISTERED;
        return ret_pair;
    }

    // Create and init registration record:
    // parse app_resource_definition and create unique access token
    RegistrationRecord_ptr registration_record = std::make_shared<RegistrationRecord>(source);
    const MblError init_status = registration_record->init(app_resource_definition);
    if (Error::None != init_status) {
        TR_ERR("registration_record->init failed with error: %s", MblError_to_str(init_status));
        if (Error::CCRBInvalidJson == init_status) {
            ret_pair.first = CloudConnectStatus::ERR_INVALID_APPLICATION_RESOURCES_DEFINITION;
            return ret_pair;
        }
        ret_pair.first = CloudConnectStatus::ERR_INTERNAL_ERROR;
        return ret_pair;
    }

    auto ret_pair_generate_access_token = ipc_adapter_->generate_access_token();
    if (Error::None != ret_pair_generate_access_token.first) {
        TR_ERR("Generate access token failed");
        ret_pair.first = CloudConnectStatus::ERR_INTERNAL_ERROR;
        return ret_pair;
    }

    // Set atomic flag for application register update in progress
    registration_record->set_registration_state(RegistrationRecord::State_RegistrationInProgress);

    // Fill ret_pair with generated access token
    ret_pair.second = ret_pair_generate_access_token.second;
    reg_update_in_progress_access_token_ = ret_pair_generate_access_token.second;

    // Add registration_record to map
    registration_records_[reg_update_in_progress_access_token_] = registration_record;

    // Call Mbed cloud client to start registration update
    mbed_client_add_objects_func_(registration_record->get_m2m_object_list());
    mbed_client_register_update_func_();

    return ret_pair;
}

CloudConnectStatus ResourceBroker::deregister_resources(IpcConnection /*source*/,
                                                        const std::string& /*access_token*/)
{
    TR_DEBUG_ENTER;
    return CloudConnectStatus::ERR_NOT_SUPPORTED;
}

CloudConnectStatus ResourceBroker::add_resource_instances(IpcConnection /*source*/,
                                                          const std::string& /*unused*/,
                                                          const std::string& /*unused*/,
                                                          const std::vector<uint16_t>& /*unused*/)
{
    TR_DEBUG_ENTER;
    return CloudConnectStatus::ERR_NOT_SUPPORTED;
}

CloudConnectStatus
ResourceBroker::remove_resource_instances(IpcConnection /*source*/,
                                          const std::string& /*unused*/,
                                          const std::string& /*unused*/,
                                          const std::vector<uint16_t>& /*unused*/)
{
    TR_DEBUG_ENTER;
    return CloudConnectStatus::ERR_NOT_SUPPORTED;
}

CloudConnectStatus
ResourceBroker::validate_resource_data(const RegistrationRecord_ptr registration_record,
                                       const ResourceData& resource_data)
{
    TR_DEBUG_ENTER;

    std::string resource_path = resource_data.get_path();
    std::pair<MblError, M2MResource*> ret_pair =
        registration_record->get_m2m_resource(resource_path);
    if (Error::None != ret_pair.first) {
        TR_ERR("get_m2m_resource failed with error: %s", MblError_to_str(ret_pair.first));
        return (ret_pair.first == Error::CCRBInvalidResourcePath)
                   ? CloudConnectStatus::ERR_INVALID_RESOURCE_PATH
                   : CloudConnectStatus::ERR_RESOURCE_NOT_FOUND;
    }

    M2MResource* m2m_resource = ret_pair.second; // Found m2m resource
    M2MResourceBase::ResourceType resource_type = m2m_resource->resource_instance_type();

    // Type validity check
    switch (resource_data.get_data_type())
    {
    case ResourceDataType::INTEGER:
        if (resource_type != M2MResourceInstance::INTEGER) {
            TR_ERR("Resource: %s - type is not integer", resource_path.c_str());
            return CloudConnectStatus::ERR_INVALID_RESOURCE_TYPE;
        }
        break;
    case ResourceDataType::STRING:
        if (resource_type != M2MResourceInstance::STRING) {
            TR_ERR("Resource: %s - type is not string", resource_path.c_str());
            return CloudConnectStatus::ERR_INVALID_RESOURCE_TYPE;
        }
        break;
    default:
        TR_ERR("Resource: %s - type not supported: %d",
               resource_path.c_str(),
               resource_data.get_data_type());
        return ERR_INVALID_RESOURCE_TYPE;
    }
    return CloudConnectStatus::STATUS_SUCCESS;
}

bool ResourceBroker::validate_set_resources_input_params(
    const RegistrationRecord_ptr registration_record,
    std::vector<ResourceSetOperation>& inout_set_operations)
{
    TR_DEBUG_ENTER;
    bool status = true;
    // Go over all resources in the vector, check for validity and update out status
    for (auto& itr : inout_set_operations) {
        itr.output_status_ = validate_resource_data(registration_record, itr.input_data_);
        if (CloudConnectStatus::STATUS_SUCCESS != itr.output_status_) {
            status = false;
        }
    }
    return status;
}

CloudConnectStatus
ResourceBroker::set_resource_value(const RegistrationRecord_ptr registration_record,
                                   const ResourceData resource_data)
{
    TR_DEBUG_ENTER;

    const std::string path = resource_data.get_path();

    // No need to check ret_pair as we already did a validity check and we know it exists
    std::pair<MblError, M2MResource*> ret_pair = registration_record->get_m2m_resource(path);
    M2MResource* m2m_resource = ret_pair.second;

    switch (resource_data.get_data_type())
    {
    case ResourceDataType::INTEGER:
    {
        int64_t value = resource_data.get_value_integer();
        if (!m2m_resource->set_value(value)) {
            TR_ERR("Set value of resource: %s to: %" PRId64 " (type: integer) failed",
                   path.c_str(),
                   value);
            return CloudConnectStatus::ERR_INTERNAL_ERROR;
        }
        TR_INFO("Set value of resource: %s to: %" PRId64 " (type: integer) succeeded.",
                path.c_str(),
                value);
        break;
    }
    case ResourceDataType::STRING:
    {
        std::string value = resource_data.get_value_string();
        const uint8_t value_length = static_cast<uint8_t>(value.length());
        std::vector<uint8_t> value_uint8(value.begin(), value.end());
        if (!m2m_resource->set_value(value_uint8.data(), value_length)) {
            TR_ERR("Set value of resource: %s to: %s (type: string) failed",
                   path.c_str(),
                   value.c_str());
            return CloudConnectStatus::ERR_INTERNAL_ERROR;
        }
        TR_INFO("Set value of resource: %s to: %s (type: string) succeeded.",
                path.c_str(),
                value.c_str());
        break;
    }
    default: // Already did a validity check so we can't be here...
        break;
    }
    return CloudConnectStatus::STATUS_SUCCESS;
}

CloudConnectStatus
ResourceBroker::set_resources_values(IpcConnection source,
                                     const std::string& access_token,
                                     std::vector<ResourceSetOperation>& inout_set_operations)
{
    TR_DEBUG("access_token: %s", access_token.c_str());

    // It is allowed to set resource value only after device is registered
    if (State_ClientRegistered != mbed_client_state_.load()) {
        TR_ERR("Client is not registered.");
        return CloudConnectStatus::ERR_INTERNAL_ERROR;
    }

    RegistrationRecord_ptr registration_record = get_registration_record(access_token);
    if (nullptr == registration_record) {
        TR_ERR("Registration record (access_token: %s) does not exist.", access_token.c_str());
        return CloudConnectStatus::ERR_INVALID_ACCESS_TOKEN;
    }

    const MblError status =
        registration_record->track_ipc_connection(source, RegistrationRecord::TrackOperation::ADD);
    if (MblError::None != status) {
        TR_ERR("track_ipc_connection failed with error: %s", MblError_to_str(status));
        return CloudConnectStatus::ERR_INTERNAL_ERROR;
    }

    // Validate all set operations and update their statuses. This is done preior to actual
    // set operation to reduce inconsistent state where some of the set operation were done and
    // some didn't.
    if (!validate_set_resources_input_params(registration_record, inout_set_operations)) {
        TR_ERR("validate_set_resources_input_params (access_token: %s) failed",
               access_token.c_str());
        return CloudConnectStatus::STATUS_SUCCESS;
    }

    // Go over all resources, set values and update out status
    for (auto& itr : inout_set_operations) {
        itr.output_status_ = set_resource_value(registration_record, itr.input_data_);
    }
    return CloudConnectStatus::STATUS_SUCCESS;
}

bool ResourceBroker::validate_get_resources_input_params(
    const RegistrationRecord_ptr registration_record,
    std::vector<ResourceGetOperation>& inout_get_operations)
{
    TR_DEBUG_ENTER;
    bool status = true;
    // Go over all resources in the vector, check for validity and update out status
    for (auto& itr : inout_get_operations) {
        itr.output_status_ = validate_resource_data(registration_record, itr.inout_data_);
        if (CloudConnectStatus::STATUS_SUCCESS != itr.output_status_) {
            status = false;
        }
    }
    return status;
}

void ResourceBroker::get_resource_value(const RegistrationRecord_ptr registration_record,
                                        ResourceData& resource_data)
{
    TR_DEBUG_ENTER;

    const std::string path = resource_data.get_path();

    // No need to check ret_pair as we already did a validity check and we know it exists
    std::pair<MblError, M2MResource*> ret_pair = registration_record->get_m2m_resource(path);
    M2MResource* m2m_resource = ret_pair.second;

    switch (resource_data.get_data_type())
    {
    case ResourceDataType::INTEGER:
    {
        int64_t value = m2m_resource->get_value_int();
        resource_data.set_value(value);
        TR_INFO("Value of resource: %s (type: integer) is: %" PRId64, path.c_str(), value);
        break;
    }
    case ResourceDataType::STRING:
    {
        std::string value = m2m_resource->get_value_string().c_str();
        resource_data.set_value(value);
        TR_INFO("Value of resource: %s (type: string) is: %s", path.c_str(), value.c_str());
        break;
    }
    default:
        assert(0); // Already did a validity check so we can't be here...
        break;
    }
}

CloudConnectStatus
ResourceBroker::get_resources_values(IpcConnection source,
                                     const std::string& access_token,
                                     std::vector<ResourceGetOperation>& inout_get_operations)
{
    TR_DEBUG_ENTER;

    TR_DEBUG("access_token: %s", access_token.c_str());

    // It is allowed to get resource value only after device is registered
    if (State_ClientRegistered != mbed_client_state_.load()) {
        TR_ERR("Client is not registered.");
        return CloudConnectStatus::ERR_INTERNAL_ERROR;
    }

    RegistrationRecord_ptr registration_record = get_registration_record(access_token);
    if (nullptr == registration_record) {
        TR_ERR("Registration record (access_token: %s) does not exist.", access_token.c_str());
        return CloudConnectStatus::ERR_INVALID_ACCESS_TOKEN;
    }

    const MblError status =
        registration_record->track_ipc_connection(source, RegistrationRecord::TrackOperation::ADD);
    if (MblError::None != status) {
        TR_ERR("track_ipc_connection failed with error: %s", MblError_to_str(status));
        return CloudConnectStatus::ERR_INTERNAL_ERROR;
    }

    // Validate all get operations and update their statuses. This is done preior to actual
    // get operation to reduce inconsistent state where some of the get operation were done and
    // some didn't.
    if (!validate_get_resources_input_params(registration_record, inout_get_operations)) {
        TR_ERR("validate_get_resources_input_params (access_token: %s) failed",
               access_token.c_str());
        return CloudConnectStatus::STATUS_SUCCESS;
    }

    // Go over all resources, get values and update out status
    for (auto& itr : inout_get_operations) {
        get_resource_value(registration_record, itr.inout_data_);
    }
    return CloudConnectStatus::STATUS_SUCCESS;
}

void ResourceBroker::notify_connection_closed(IpcConnection source)
{
    TR_DEBUG_ENTER;

    auto itr = registration_records_.begin();
    while (itr != registration_records_.end()) {
        // Call track_ipc_connection, and in case registration record does not have any other
        // ipc connections - erase from registration_records_
        const MblError status =
            itr->second->track_ipc_connection(source, RegistrationRecord::TrackOperation::REMOVE);
        if (Error::CCRBNoValidConnection == status) {
            // Registration record does not have any more valid connections - erase from map
            TR_DEBUG("Erase registration record (access_token: %s)", itr->first.c_str());
            itr = registration_records_.erase(itr); // supported since C++11
        }
        else
        {
            itr++;
        }
    }
}

} // namespace mbl
