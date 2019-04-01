/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ResourceBroker.h"
#include "CloudConnectTrace.h"
#include "CloudConnectTypes.h"
#include "DBusAdapter.hpp"
#include "MailboxMsg.h"
#include "MbedClientManager.h"

#include <cassert>
#include <time.h>
#include <unistd.h>

#define TRACE_GROUP "ccrb"

namespace mbl
{

// Period between re-registrations with the LWM2M server.
// MBED_CLOUD_CLIENT_LIFETIME (seconds) is how long we should stay registered after each
// re-registration (keepalive)
static const uint64_t g_keepalive_period_miliseconds = (MBED_CLOUD_CLIENT_LIFETIME / 2) * 1000;

/**
 * @brief Mailbox message for mbed client RegistrationUpdated callback
 * This message is sent to mailbox when mbed client RegistrationUpdated callback is called
 * In order to handle the callback in internal therad and not mbed client thread
 */
struct MailboxMsg_RegistrationUpdated
{
    MblError status;
};

/**
 * @brief Mailbox message for mbed client error callback
 * This message is sent to mailbox when mbed client error callback is called
 * In order to handle the callback in internal therad and not mbed client thread
 */
struct MailboxMsg_MbedClientError
{
    MblError status;
};

MblError ResourceBroker::main()
{
    TR_DEBUG_ENTER;
    ResourceBroker resource_broker;

    TR_INFO("Init CCRB");
    OneSetMblError status(resource_broker.init());
    if (Error::None != status.get()) {
        // Not returning as we want to call deinit below
        TR_ERR("CCRB::init failed with error %s", status.get_status_str());
    }

    // Call ccrb->run only if init succeeded
    if (Error::None == status.get()) {
        
        assert(resource_broker.ipc_adapter_);
        
        MblError stop_status = Error::None;
        TR_INFO("Run IPC adapter");
        status.set(resource_broker.ipc_adapter_->run(stop_status));
        if (Error::None != status.get()) {
            TR_ERR("Run IPC adapter failed with error %s", status.get_status_str());
        } else {

            TR_INFO("Run IPC adapter successfully stopped");

            resource_broker.mbed_client_manager_->unregister_mbed_client();

            // Wait for mbed client manager to finish device unregistration
            for (;;) {
                if (MbedClientManager::State_DeviceUnregistered ==
                    resource_broker.mbed_client_manager_->get_device_state())
                {
                    break;
                }
                sleep(1);
            }
        }
    }

    TR_INFO("Deinit CCRB");
    const MblError deinit_status = resource_broker.deinit();
    if (Error::None != deinit_status) {
        TR_ERR("Deinit CCRB failed with error %s", MblError_to_str(deinit_status));
        status.set(deinit_status);
    }

    return Error::ShutdownRequested;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// This constructor is called from main function
ResourceBroker::ResourceBroker()
    : init_sem_initialized_(false),
      ipc_adapter_(nullptr),
      mbed_client_manager_(std::make_unique<MbedClientManager>())
{
    TR_DEBUG_ENTER;
}

ResourceBroker::~ResourceBroker()
{
    TR_DEBUG_ENTER;
}

MblError ResourceBroker::periodic_keepalive_callback(sd_event_source* s, Event* ev)
{
    TR_DEBUG_ENTER;
    UNUSED(s); // Can't call unref_event_source as this event should be keep on comming
    assert(ev);
    EventPeriodic* periodic_ev = dynamic_cast<EventPeriodic*>(ev);
    if (nullptr == periodic_ev) {
        TR_ERR("Invalid periodic event type");
        return MblError::SystemCallFailed;
    }

    TR_DEBUG("%s event", periodic_ev->get_description().c_str());

    auto unpack_pair = periodic_ev->unpack_data<EventData_Keepalive>(sizeof(EventData_Keepalive));
    if (unpack_pair.first != Error::None) {
        TR_ERR("Unpack of periodic event failed with error %s", MblError_to_str(unpack_pair.first));
        return unpack_pair.first;
    }

    ResourceBroker* const this_ccrb = unpack_pair.second.ccrb_this;
    assert(this_ccrb);

    // Keep alive is only needed when device is registered
    if (MbedClientManager::State_DeviceRegistered !=
        this_ccrb->mbed_client_manager_->get_device_state())
    {
        TR_DEBUG("Device is not registered.");
        return Error::None;
    }

    // Check if application register update is in progress
    // if yes - do nothing as register update will act as keep alive
    if (!this_ccrb->reg_update_in_progress_access_token_.empty()) {
        TR_DEBUG("Application registration update is in progress- no need for keepalive.");
        return Error::None;
    }

    TR_DEBUG("Call cloud_client_->register_update for (keepalive)");
    this_ccrb->mbed_client_manager_->keepalive();

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

    mbed_client_manager_->init();

    // Init Mbed cloud client
    mbed_client_manager_->set_resources_registration_succeeded_callback(
        std::bind(&ResourceBroker::resources_registration_succeeded, this));

    mbed_client_manager_->set_mbed_client_error_callback(std::bind(
        static_cast<void (ResourceBroker::*)(MblError)>(&ResourceBroker::handle_mbed_client_error),
        this,
        std::placeholders::_1));

    const MblError register_mbed_client_status = mbed_client_manager_->register_mbed_client();
    if (Error::None != register_mbed_client_status) {
        TR_ERR("mbed_client_manager_->register_mbed_client() failed with error %s",
               MblError_to_str(register_mbed_client_status));
        return register_mbed_client_status;
    }

    // Set keepalive periodic event
    EventData_Keepalive event_data = {this};
    auto ret_pair = ipc_adapter_->send_event_periodic<EventData_Keepalive>(
        event_data,
        sizeof(event_data),
        ResourceBroker::periodic_keepalive_callback,
        g_keepalive_period_miliseconds,
        std::string("Mbed cloud client keep-alive"));

    if (Error::None != ret_pair.first) {
        TR_ERR("send_event_periodic keep-alive failed with error %s",
               MblError_to_str(ret_pair.first));
        return ret_pair.first;
    }

    return Error::None;
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
    mbed_client_manager_->deinit();

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

    if (access_token.empty()) {
        TR_ERR("access_token is empty");
        return nullptr;
    }

    auto itr = registration_records_.find(access_token);
    if (registration_records_.end() == itr) {
        // Could not found registration_record
        TR_ERR("Registration record (access_token: %s) does not exist.", access_token.c_str());
        return nullptr;
    }

    return itr->second;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Mailbox messages related functions
////////////////////////////////////////////////////////////////////////////////////////////////////

MblError ResourceBroker::handle_mbed_client_error_internal_message(MblError mbed_client_error)
{
    // Check if this callback is caused by an application trying to register resources
    // (only one application can be in registration update state at a time)
    RegistrationRecord_ptr registration_record =
        get_registration_record(reg_update_in_progress_access_token_);
    if (nullptr != registration_record) {

        TR_ERR("Registration (access_token: %s) failed with error: %s",
               reg_update_in_progress_access_token_.c_str(),
               MblError_to_str(mbed_client_error));

        const MblError update_status = ipc_adapter_->update_registration_status(
            registration_record->get_registration_source(), CloudConnectStatus::ERR_INTERNAL_ERROR);
        if (Error::None != update_status) {
            TR_ERR("ipc_adapter_->update_registration_status failed with error: %s",
                   MblError_to_str(update_status));
        }

        TR_DEBUG("Erase registration record (access_token: %s)",
                 reg_update_in_progress_access_token_.c_str());

        auto itr = registration_records_.find(reg_update_in_progress_access_token_);
        registration_records_.erase(itr); // Erase registration record as regitsration failed
        reg_update_in_progress_access_token_.clear();
        return update_status;
    }

    // Keepalive:
    TR_ERR("Keepalive request failed with error: %s", MblError_to_str(mbed_client_error));
    return MblError::None;
}

MblError ResourceBroker::handle_resources_registration_succeeded_internal_message()
{
    TR_DEBUG_ENTER;

    // This function can be called as a result of:
    // 1. application requested to register resources
    // 2. keep alive (does not occur during #1)

    // Only one application can be in registration update state - check if there is such...
    RegistrationRecord_ptr registration_record =
        get_registration_record(reg_update_in_progress_access_token_);
    if (nullptr != registration_record) {

        TR_DEBUG("Registration record (access_token: %s) registered successfully.",
                 reg_update_in_progress_access_token_.c_str());

        registration_record->set_registration_state(RegistrationRecord::State_Registered);

        // Send the response to adapter:
        const MblError status = ipc_adapter_->update_registration_status(
            registration_record->get_registration_source(), CloudConnectStatus::STATUS_SUCCESS);
        if (Error::None != status) {
            TR_ERR("update_registration_status failed with error: %s", MblError_to_str(status));
        }
        reg_update_in_progress_access_token_.clear();
        return status;
    }

    TR_DEBUG("Keepalive finished successfully");
    return Error::None;
}

MblError ResourceBroker::process_mailbox_message(MailboxMsg& msg)
{
    TR_DEBUG_ENTER;

    auto data_type_name = msg.get_data_type_name();

    // Mbed Client Registration Updated
    if (data_type_name == typeid(MailboxMsg_RegistrationUpdated).name()) {
        TR_INFO("Process message MailboxMsg_RegistrationUpdated");
        MblError status;
        MailboxMsg_RegistrationUpdated message;
        std::tie(status, message) =
            msg.unpack_data<MailboxMsg_RegistrationUpdated>(sizeof(MailboxMsg_RegistrationUpdated));
        if (status != MblError::None) {
            TR_ERR("msg.unpack_data failed with error: %s", MblError_to_str(status));
            return Error::DBA_MailBoxInvalidMsg;
        }
        return handle_resources_registration_succeeded_internal_message();
    }

    // Mbed Client Error
    if (data_type_name == typeid(MailboxMsg_MbedClientError).name()) {
        TR_INFO("Process message MailboxMsg_MbedClientError");
        MblError status;
        MailboxMsg_MbedClientError message;
        std::tie(status, message) =
            msg.unpack_data<MailboxMsg_MbedClientError>(sizeof(MailboxMsg_MbedClientError));
        if (status != MblError::None) {
            TR_ERR("msg.unpack_data failed with error: %s", MblError_to_str(status));
            return Error::DBA_MailBoxInvalidMsg;
        }
        return handle_mbed_client_error_internal_message(message.status);
    }

    // This should never happen
    TR_WARN("Unexpected message type %s, Ignoring...", data_type_name.c_str());
    return Error::None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// APIs to be used by Mbed client manager class
////////////////////////////////////////////////////////////////////////////////////////////////////
void ResourceBroker::resources_registration_succeeded()
{
    TR_DEBUG_ENTER;

    // Send mailbox message (will be handled in process_mailbox_message API)
    MailboxMsg_RegistrationUpdated registration_updated_msg;
    registration_updated_msg.status = MblError::None;
    MailboxMsg msg(registration_updated_msg, sizeof(registration_updated_msg));
    TR_ERR("send_mailbox_msg for register update");
    const MblError send_status = ipc_adapter_->send_mailbox_msg(msg);
    if (send_status != MblError::None) {
        TR_ERR("send_mailbox_msg failed with error %s", MblError_to_str(send_status));
    }
}

void ResourceBroker::handle_mbed_client_error(MblError cloud_client_error)
{
    TR_DEBUG_ENTER;

    // Send mailbox message (will be handled in process_mailbox_message API)
    MailboxMsg_MbedClientError mbed_client_error_msg;
    mbed_client_error_msg.status = cloud_client_error;
    MailboxMsg msg(mbed_client_error_msg, sizeof(mbed_client_error_msg));
    TR_ERR("send_mailbox_msg for mbed client error");
    const MblError send_status = ipc_adapter_->send_mailbox_msg(msg);
    if (send_status != MblError::None) {
        TR_ERR("send_mailbox_msg failed with error %s", MblError_to_str(send_status));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// APIs to be used by DBusAdapter class
////////////////////////////////////////////////////////////////////////////////////////////////////

std::pair<CloudConnectStatus, std::string>
ResourceBroker::register_resources(IpcConnection source, const std::string& app_resource_definition)
{
    TR_DEBUG_ENTER;

    std::pair<CloudConnectStatus, std::string> ret_pair(CloudConnectStatus::STATUS_SUCCESS,
                                                        std::string());

    if (MbedClientManager::State_DeviceRegistered != mbed_client_manager_->get_device_state()) {
        TR_ERR("Client is not registered.");
        ret_pair.first = CloudConnectStatus::ERR_INTERNAL_ERROR;
        return ret_pair;
    }

    // Only one register update request is allowed at a time:
    if (!reg_update_in_progress_access_token_.empty()) {
        TR_ERR("Registration of resources is already in progress.");
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
    mbed_client_manager_->register_resources(registration_record->get_m2m_object_list());
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
    auto ret_pair = registration_record->get_m2m_resource(resource_path);
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
    auto ret_pair = registration_record->get_m2m_resource(path);
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
    if (MbedClientManager::State_DeviceRegistered != mbed_client_manager_->get_device_state()) {
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
    auto ret_pair = registration_record->get_m2m_resource(path);
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
    if (MbedClientManager::State_DeviceRegistered != mbed_client_manager_->get_device_state()) {
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
        // this is safe as all related operation on registration_records_ are done in internal
        // thread.
        const MblError status =
            itr->second->track_ipc_connection(source, RegistrationRecord::TrackOperation::REMOVE);
        // Check if Registration record does not have any more valid connections - if yes
        // erase from map
        if (Error::CCRBNoValidConnection == status) {
            // Check if registration record is in progress of register update its resources
            // if yes - clear reg_update_in_progress_access_token_ member
            if (itr->first == reg_update_in_progress_access_token_) {
                TR_WARN("Erase registration record (access_token: %s) during register of resources",
                        itr->first.c_str());
                reg_update_in_progress_access_token_.clear();
            }

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
