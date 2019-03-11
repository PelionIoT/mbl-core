/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ResourceBroker.h"
#include "CloudConnectTrace.h"
#include "CloudConnectTypes.h"
#include "mbed-cloud-client/MbedCloudClient.h"

#include <cassert>
#include <time.h>

#define TRACE_GROUP "ccrb"

namespace mbl
{

// Currently, this constructor is called from MblCloudClient thread.
ResourceBroker::ResourceBroker()
    : init_sem_initialized_(false), cloud_client_(nullptr), registration_in_progress_(false)
{
    TR_DEBUG_ENTER;
}

ResourceBroker::~ResourceBroker()
{
    TR_DEBUG_ENTER;
}

MblError ResourceBroker::start(MbedCloudClient* cloud_client)
{
    // FIXME: This function should be refactored  in IOTMBL-1707.
    // Initialization of the semaphore and call to the sem_timedwait will be removed.

    TR_DEBUG_ENTER;
    // assert(cloud_client);     // TODO: uncomment after solving PAL issues on Linux Desktop
    cloud_client_ = cloud_client;

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
        else
        {
            TR_ERRNO("sem_timedwait", errno);
            return Error::SystemCallFailed;
        }
    }
    else
    {
        // init_sem_ was signalled
        TR_INFO("Resource Broker initializations finished successfully");
    }

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

MblError ResourceBroker::init()
{
    TR_DEBUG_ENTER;

    // verify that ipc_adapter_ member was not created yet
    assert(nullptr == ipc_adapter_);

    // create ipc instance and pass ccrb instance to constructor
    ipc_adapter_ = std::make_unique<DBusAdapter>(*this);

    OneSetMblError status(ipc_adapter_->init());
    if (Error::None != status.get()) {
        TR_ERR("ipc::init failed with error %s", status.get_status_str());
    }

    // Set function pointers to point to mbed_client functions
    // In gtest our friend test class will override these pointers to get all the
    // calls
    register_update_func_ = std::bind(&MbedCloudClient::register_update, cloud_client_);
    add_objects_func_ = std::bind(
        static_cast<void (MbedCloudClient::*)(const M2MObjectList&)>(&MbedCloudClient::add_objects),
        cloud_client_,
        std::placeholders::_1);

    // Register Cloud client callback:
    regsiter_callback_handlers();

    //////////////////////////////////////////////////////////////////
    // PAY ATTENTION: This should be the last action in this function!
    //////////////////////////////////////////////////////////////////
    // signal to the semaphore that initialization was finished
    const int ret = sem_post(&init_sem_);
    if (0 != ret) {
        // semaphore post failed, print errno value and exit
        TR_ERRNO("sem_post", errno);
        status.set(Error::IpcProcedureFailed);
    }

    return status.get();
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
    }
    else
    {
        TR_ERR("ipc::run stopped with status %s", MblError_to_str(stop_status));
    }

    return status;
}

void* ResourceBroker::ccrb_main(void* ccrb)
{
    TR_DEBUG_ENTER;

    assert(ccrb);

    ResourceBroker* const this_ccrb = static_cast<ResourceBroker*>(ccrb);

    const MblError init_status = this_ccrb->init();
    if (Error::None != init_status) {
        TR_ERR("CCRB::init failed with error %s. Exit CCRB thread.", MblError_to_str(init_status));
        uintptr_t int_init_status = static_cast<uintptr_t>(init_status);
        pthread_exit(reinterpret_cast<void*>(int_init_status)); // NOLINT
    }

    const MblError run_status = this_ccrb->run();
    if (Error::None != run_status) {
        TR_ERR("CCRB::run failed with error %s. Exit CCRB thread.", MblError_to_str(run_status));
        // continue to deinit and return status
    }

    const MblError deinit_status = this_ccrb->deinit();
    if (Error::None != deinit_status) {
        TR_ERR("CCRB::deinit failed with error %s. Exit CCRB thread.",
               MblError_to_str(deinit_status));
        uintptr_t int_deinit_status = static_cast<uintptr_t>(deinit_status);
        pthread_exit(reinterpret_cast<void*>(int_deinit_status)); // NOLINT
    }

    TR_INFO("CCRB thread function finished. CCRB::run status %s.", MblError_to_str(run_status));
    uintptr_t int_run_status = static_cast<uintptr_t>(run_status);

    // pthread_exit does "return"
    pthread_exit(reinterpret_cast<void*>(int_run_status)); // NOLINT
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Callback functions that are being called by Mbed client
////////////////////////////////////////////////////////////////////////////////////////////////////
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

void ResourceBroker::regsiter_callback_handlers()
{
    TR_DEBUG_ENTER;

    // Resource broker will now get Mbed cloud client callbacks on the following
    // cases:
    // 1. Registration update
    // 2. Error occurred

    if (nullptr != cloud_client_) {
        // GTest unit test might set cloud_client_ member to nullptr
        cloud_client_->on_registration_updated(this,
                                               &ResourceBroker::handle_registration_updated_cb);
        cloud_client_->on_error(this, &ResourceBroker::handle_error_cb);
    }
}

void ResourceBroker::handle_registration_updated_cb()
{
    TR_DEBUG_ENTER;

    // TODO: need to handle cases when mbed client does not call any callback
    // during registration - IOTMBL-1700

    if (registration_in_progress_.load()) {

        RegistrationRecord_ptr registration_record =
            get_registration_record(in_progress_access_token_);
        if (nullptr == registration_record) {
            // Could not found registration record
            TR_ERR("Registration record (access_token: %s) does not exist.",
                   in_progress_access_token_.c_str());
            // Mark that registration is finished (using atomic flag), even if it was failed
            registration_in_progress_.store(false);
            return;
        }

        TR_DEBUG("Registration record (access_token: %s) registered successfully.",
                 in_progress_access_token_.c_str());

        registration_record->set_registered(true);

        // Send the response to adapter:
        CloudConnectStatus reg_status = CloudConnectStatus::STATUS_SUCCESS;
        const MblError status = ipc_adapter_->update_registration_status(
            registration_record->get_connection(), reg_status);
        if (Error::None != status) {
            TR_ERR("update_registration_status failed with error: %s", MblError_to_str(status));
        }

        // Mark that registration is finished (using atomic flag)
        registration_in_progress_.store(false);
    }
    else
    {
        TR_DEBUG("handle_registration_updated_cb was called as a result of keep-alive request");
    }
}

// TODO: this callback is related to the following scenarios which are not yet implemented:
// keep alive, deregistration, add resource instances and remove resource instances.
void ResourceBroker::handle_error_cb(const int cloud_client_code)
{
    TR_DEBUG_ENTER;

    const MblError mbl_code =
        CloudClientError_to_MblError(static_cast<MbedCloudClient::Error>(cloud_client_code));
    TR_ERR("Error occurred: %d: %s", mbl_code, MblError_to_str(mbl_code));

    // If registration is in progress - update adapter
    if (registration_in_progress_.load()) {

        TR_DEBUG("Registration (access_token: %s) failed.", in_progress_access_token_.c_str());

        RegistrationRecord_ptr registration_record =
            get_registration_record(in_progress_access_token_);
        if (nullptr == registration_record) {
            // Could not found registration record
            TR_ERR("Registration record (access_token: %s) does not exist.",
                   in_progress_access_token_.c_str());
            // Mark that registration is finished (using atomic flag), even if it was failed
            registration_in_progress_.store(false);
            return;
        }

        // Send the response to adapter:
        if (registration_record->is_registered()) {
            // TODO: add call to update_deregistration_status when deregister is implemented
            TR_ERR("Registration record (access_token: %s) is already registered.",
                   in_progress_access_token_.c_str());
        }
        else
        {
            // Registration record is not registered yet, which means the error is for register
            // request
            const MblError status = ipc_adapter_->update_registration_status(
                registration_record->get_connection(), CloudConnectStatus::ERR_INTERNAL_ERROR);
            if (Error::None != status) {
                TR_ERR("ipc_adapter_->update_registration_status failed with error: %s",
                       MblError_to_str(status));
            }

            TR_DEBUG("Erase registration record (access_token: %s)",
                     in_progress_access_token_.c_str());
            auto itr = registration_records_.find(in_progress_access_token_);
            registration_records_.erase(itr); // Erase registration record as regitsration failed
        }
        // Mark that registration is finished (using atomic flag) (even if it was failed)
        registration_in_progress_.store(false);
    }
    else
    {
        TR_DEBUG("handle_error_cb was called as a result of keep-alive request");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

MblError ResourceBroker::register_resources(const IpcConnection& source,
                                            const std::string& app_resource_definition_json,
                                            CloudConnectStatus& out_status,
                                            std::string& out_access_token)
{
    TR_DEBUG_ENTER;

    if (registration_in_progress_.load()) {
        // We only allow one registration request at a time.
        TR_ERR("Registration is already in progress.");
        out_status = CloudConnectStatus::ERR_REGISTRATION_ALREADY_IN_PROGRESS;
        return Error::None;
    }

    // Above check makes sure there is no registration in progress.
    // Lets check if an app is already not registered...
    // TODO: remove this check when supporting multiple applications
    if (!registration_records_.empty()) {
        // Currently support only ONE application
        TR_ERR("Only one registration is allowed.");
        out_status = CloudConnectStatus::ERR_ALREADY_REGISTERED;
        return Error::None;
    }

    // Create and init registration record:
    // parse app_resource_definition_json and create unique access token
    RegistrationRecord_ptr registration_record = std::make_shared<RegistrationRecord>(source);
    const MblError init_status = registration_record->init(app_resource_definition_json);
    if (Error::None != init_status) {
        TR_ERR("registration_record->init failed with error: %s", MblError_to_str(init_status));
        if (Error::CCRBInvalidJson == init_status) {
            out_status = CloudConnectStatus::ERR_INVALID_APPLICATION_RESOURCES_DEFINITION;
            return Error::None;
        }
        return init_status;
    }

    auto ret_pair = ipc_adapter_->generate_access_token();
    if (Error::None != ret_pair.first) {
        TR_ERR("Generate access token failed with error: %s", MblError_to_str(ret_pair.first));
        return ret_pair.first;
    }

    // Set atomic flag for registration in progress
    registration_in_progress_.store(true);

    out_access_token = ret_pair.second;
    in_progress_access_token_ = ret_pair.second;

    registration_records_[out_access_token] = registration_record; // Add registration_record to map

    // Call Mbed cloud client to start registration update
    add_objects_func_(registration_record->get_m2m_object_list());
    register_update_func_();

    out_status = CloudConnectStatus::STATUS_SUCCESS;

    return Error::None;
}

MblError ResourceBroker::deregister_resources(const IpcConnection& /*source*/,
                                              const std::string& /*access_token*/,
                                              CloudConnectStatus& /*out_status*/)
{

    TR_DEBUG_ENTER;
    // empty for now
    return Error::CCRBNotSupported;
}

MblError ResourceBroker::add_resource_instances(const IpcConnection& /*source*/,
                                                const std::string& /*unused*/,
                                                const std::string& /*unused*/,
                                                const std::vector<uint16_t>& /*unused*/,
                                                CloudConnectStatus& /*unused*/)
{
    TR_DEBUG_ENTER;
    // empty for now
    return Error::None;
}

MblError ResourceBroker::remove_resource_instances(const IpcConnection& /*source*/,
                                                   const std::string& /*unused*/,
                                                   const std::string& /*unused*/,
                                                   const std::vector<uint16_t>& /*unused*/,
                                                   CloudConnectStatus& /*unused*/)
{
    TR_DEBUG_ENTER;
    // empty for now
    return Error::None;
}

CloudConnectStatus
ResourceBroker::validate_resource_data(const RegistrationRecord_ptr registration_record,
                                       ResourceData resource_data)
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

        ResourceSetOperation set_operation = itr;
        const ResourceData input_data = set_operation.input_data_;

        itr.output_status_ = validate_resource_data(registration_record, input_data);
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

MblError
ResourceBroker::set_resources_values(const IpcConnection& /*source*/,
                                     const std::string& access_token,
                                     std::vector<ResourceSetOperation>& inout_set_operations,
                                     CloudConnectStatus& out_status)
{
    TR_DEBUG("access_token: %s", access_token.c_str());

    RegistrationRecord_ptr registration_record = get_registration_record(access_token);
    if (nullptr == registration_record) {
        TR_ERR("Registration record (access_token: %s) does not exist.", access_token.c_str());
        out_status = CloudConnectStatus::ERR_INVALID_ACCESS_TOKEN;
        return Error::None;
    }

    // Validate all set operations and update their statuses. This is done preior to actual
    // set operation to reduce inconsistent state where some of the set operation were done and
    // some didn't.
    if (!validate_set_resources_input_params(registration_record, inout_set_operations)) {
        TR_ERR("validate_set_resources_input_params (access_token: %s) failed",
               access_token.c_str());
        out_status = STATUS_SUCCESS;
        return Error::None;
    }

    // Go over all resources, set values and update out status
    for (auto& itr : inout_set_operations) {
        ResourceSetOperation set_operation = itr;
        itr.output_status_ = set_resource_value(registration_record, set_operation.input_data_);
    }
    out_status = CloudConnectStatus::STATUS_SUCCESS;
    return Error::None;
}

bool ResourceBroker::validate_get_resources_input_params(
    RegistrationRecord_ptr registration_record,
    std::vector<ResourceGetOperation>& inout_get_operations)
{
    TR_DEBUG_ENTER;
    bool status = true;
    // Go over all resources in the vector, check for validity and update out status
    for (auto& itr : inout_get_operations) {

        ResourceGetOperation get_operation = itr;
        ResourceData inout_data = get_operation.inout_data_;
        const std::string path = inout_data.get_path();
        itr.output_status_ = validate_resource_data(registration_record, inout_data);
        if (CloudConnectStatus::STATUS_SUCCESS != itr.output_status_) {
            status = false;
        }
    }
    return status;
}

MblError
ResourceBroker::get_resources_values(const IpcConnection& /*source*/,
                                     const std::string& access_token,
                                     std::vector<ResourceGetOperation>& inout_get_operations,
                                     CloudConnectStatus& out_status)
{
    TR_DEBUG_ENTER;

    TR_DEBUG("access_token: %s", access_token.c_str());

    RegistrationRecord_ptr registration_record = get_registration_record(access_token);
    if (nullptr == registration_record) {
        TR_ERR("Registration record (access_token: %s) does not exist.", access_token.c_str());
        out_status = CloudConnectStatus::ERR_INVALID_ACCESS_TOKEN;
        return Error::None;
    }

    // Validate all get operations and update their statuses. This is done preior to actual
    // get operation to reduce inconsistent state where some of the get operation were done and
    // some didn't.
    if (!validate_get_resources_input_params(registration_record, inout_get_operations)) {
        TR_ERR("validate_get_resources_input_params (access_token: %s) failed",
               access_token.c_str());
        out_status = STATUS_SUCCESS;
        return Error::None;
    }

    // IMPLEMENTATION IN PROGRESS
    return Error::None;
}

void ResourceBroker::notify_connection_closed(const IpcConnection& /*source*/)
{
    TR_DEBUG_ENTER;
    // TODO:implement
    assert(0);
}

} // namespace mbl
