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

#include "MbedClientManager.h"
#include "CloudConnectTrace.h"

#include "mbed-cloud-client/MbedCloudClient.h"
#include "mbed_cloud_client_user_config.h"
#include "ns-hal-pal/ns_event_loop.h"

#define TRACE_GROUP "ccrb-mbed-client-mng"

namespace mbl
{

MbedClientManager* MbedClientManager::s_instance = nullptr;
uint32_t MbedClientManager::dummy_network_interface_ = 0xFFFFFFFF;

MbedClientManager::MbedClientManager()
    : mbed_client_state_(State_DeviceUnregistered),
      cloud_client_(nullptr),
      resources_registration_succeeded_callback_func_(nullptr),
      mbed_client_error_callback_func_(nullptr),
      initializer_thread_id_(0)
{
    TR_DEBUG_ENTER;

    assert(s_instance == nullptr);
    s_instance = this;
}

MbedClientManager::~MbedClientManager()
{
    TR_DEBUG_ENTER;

    s_instance = nullptr;
}

void MbedClientManager::set_resources_registration_succeeded_callback(
    ResourcesRegistrationSucceededCallback callback_func)
{
    // Must be first! only CCRB initializer thread should call this function
    // This will prevent changing callbacks accidentally from unauthorized places
    assert(pthread_equal(pthread_self(), initializer_thread_id_) != 0);    
    resources_registration_succeeded_callback_func_ = callback_func;
}

void MbedClientManager::set_mbed_client_error_callback(MbedClientErrorCallback callback_func)
{
    // Must be first! only CCRB initializer thread should call this function
    // This will prevent changing callbacks accidentally from unauthorized places
    assert(pthread_equal(pthread_self(), initializer_thread_id_) != 0);    
    mbed_client_error_callback_func_ = callback_func;
}

void MbedClientManager::init()
{
    TR_DEBUG_ENTER;

    assert(nullptr == cloud_client_);

    // Record initializer thread ID, that should be CCRB main thread
    initializer_thread_id_ = pthread_self();

    cloud_client_ = std::make_unique<MbedCloudClient>();

    // Register mbed client callback:
    cloud_client_->on_registered(this, &MbedClientManager::handle_mbed_client_registered);
    cloud_client_->on_unregistered(this, &MbedClientManager::handle_mbed_client_unregistered);
    cloud_client_->on_registration_updated(
        this, &MbedClientManager::handle_mbed_client_registration_updated);
    cloud_client_->set_update_progress_handler(&update_handlers::handle_download_progress);
    cloud_client_->set_update_authorize_handler(&handle_mbed_client_authorize);
    cloud_client_->on_error(this, &MbedClientManager::handle_mbed_client_error);
}

void MbedClientManager::deinit()
{
    TR_DEBUG_ENTER;

    // Must call unregister_mbed_client() before deinit!
    // This also assure that no more callbacks will be called unexpectedly
    assert(State_DeviceUnregistered == mbed_client_state_.load());

    if (cloud_client_) {
        TR_INFO("Erase mbed client");
        MbedCloudClient* cloud_client_obj = cloud_client_.release();
        assert(cloud_client_obj);
        delete cloud_client_obj;
        cloud_client_ = nullptr;

        TR_INFO("Stop the PAL event loop thread");
        ns_event_loop_thread_stop();
    }
}

MblError MbedClientManager::register_mbed_client()
{
    // Register Device Default Resources
    M2MObjectList objs; // Using empty ObjectList
    cloud_client_->add_objects(objs);

    // Dummy network interface needed by cloud_client_->setup API (used only in MbedOS)
    const bool setup_ok = cloud_client_->setup(static_cast<void*>(&dummy_network_interface_));
    if (!setup_ok) {
        TR_ERR("Mbed cloud client setup failed");
        deinit();
        return Error::ConnectUnknownError; // state_ will stay on State_DeviceUnregistered
    }

    mbed_client_state_.store(State_DeviceRegisterInProgress);
    return Error::None;    
}

void MbedClientManager::unregister_mbed_client()
{
    TR_DEBUG_ENTER;

    assert(cloud_client_);
    mbed_client_state_.store(State_DeviceUnregisterInProgress);
    TR_INFO("Close mbed client");
    cloud_client_->close();
}

void MbedClientManager::keepalive()
{
    TR_DEBUG_ENTER;

    assert(cloud_client_);
    cloud_client_->register_update();
}

void MbedClientManager::register_resources(const M2MObjectList& object_list)
{
    TR_DEBUG_ENTER;

    assert(cloud_client_);
    cloud_client_->add_objects(object_list);
    cloud_client_->register_update();
}

MbedClientManager::MbedClientDeviceState MbedClientManager::get_device_state()
{
    return mbed_client_state_.load();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Callback functions that are being called by Mbed client
////////////////////////////////////////////////////////////////////////////////////////////////////

void MbedClientManager::handle_mbed_client_registered()
{
    TR_DEBUG_ENTER;

    // In case terminate signal was received during register request - ignore registration flow
    // and continue un-registering device
    // Next NOLINT filter clang-tidy warning of gcc/x86_64-linux-gnu/5.4.0 of atomic impl
    if (State_DeviceUnregisterInProgress == mbed_client_state_.load()) { // NOLINT
        TR_WARN("client_registered callback was called while trying to un-register.");
        return;
    }

    mbed_client_state_.store(State_DeviceRegistered);

    TR_INFO("Client registered");

    const ConnectorClientEndpointInfo* endpoint = cloud_client_->endpoint_info();
    if (nullptr == endpoint) {
        TR_WARN("Failed to get endpoint info");
        return;
    }

    TR_INFO("Endpoint Name: %s", endpoint->endpoint_name.c_str());
    TR_INFO("Device Id: %s", endpoint->internal_endpoint_name.c_str());
}

void MbedClientManager::handle_mbed_client_unregistered()
{
    TR_DEBUG_ENTER;

    TR_INFO("Client unregistered");
    mbed_client_state_.store(State_DeviceUnregistered);
}

void MbedClientManager::handle_mbed_client_authorize(const int32_t request)
{
    TR_DEBUG_ENTER;

    assert(s_instance != nullptr);
    assert(s_instance->cloud_client_ != nullptr);
    if (update_handlers::handle_authorize(request)) {
        s_instance->cloud_client_->update_authorize(request);
    }
}

void MbedClientManager::handle_mbed_client_registration_updated()
{
    TR_DEBUG_ENTER;

    // TODO: need to handle cases when mbed client does not call any callback
    // during registration of resources - IOTMBL-1700

    // Notify resource broker that resources registration is finished
    if (resources_registration_succeeded_callback_func_) {
        resources_registration_succeeded_callback_func_();
    }
}

bool MbedClientManager::is_action_needed_for_error(MblError mbed_client_error)
{
    TR_DEBUG_ENTER;

    switch (mbed_client_error)
    {
    // Security object is not valid or server rejects the registration.
    // No internal recovery mechanism. Actions needed on the application side.
    // This error is returned in case invalid parameters were send to mbed client add_objects()
    // Api, and as a result - setup() or register_update() API might fail.
    // if this error is returned in mbed client error callback - it means that operation failed.
    case MblError::ConnectInvalidParameters:

    // Cannot unregister as client is not registered.
    // No internal recovery mechanism. Actions needed on the application side.
    // if this error is returned in mbed client error callback - it means that unregister
    // operation (mbed client close() API) failed.
    case MblError::ConnectNotRegistered:

    // API call is not allowed for now.
    // Application should try again later
    // This error can be triggered by register_update() (keepalive/ app registering resources)
    // TODO: Need to notify application - see IOTMBL-1667
    case MblError::ConnectNotAllowed:
    {
        // All the above errors state that an action is needed
        return true;
    }
    default:
    {
        // All other errors state that an action is NOT needed (mbed client recovers automatically)
        return false;
    }
    }
}

bool MbedClientManager::is_fatal_error(MblError mbed_client_error)
{
    TR_DEBUG_ENTER;

    switch (mbed_client_error)
    {
    // Memory allocation has failed.
    // No internal recovery mechanism. Actions needed on the application side.
    // TODO: Need to notify application - see IOTMBL-1682
    case MblError::ConnectMemoryConnectFail:

    // Failed to read credentials from storage.
    // No internal recovery mechanism. Actions needed on the application side.
    // TODO: Need to notify application - see IOTMBL-1667
    case MblError::ConnectorFailedToReadCredentials:
    {
        // All the above errors state that error is fatal
        return true;
    }
    default:
    {
        // All other errors state that errors are not fatal
        return false;
    }
    }
}

// TODO: this callback is related to the following scenarios which are not yet implemented:
// add resource instances and remove resource instances.
void MbedClientManager::handle_mbed_client_error(const int cloud_client_code)
{
    TR_DEBUG_ENTER;

    const MblError mbl_code =
        CloudClientError_to_MblError(static_cast<MbedCloudClient::Error>(cloud_client_code));
    TR_ERR("Error occurred: %d: %s", mbl_code, MblError_to_str(mbl_code));
    TR_ERR("Error details: %s", cloud_client_->error_description());

    if (is_fatal_error(mbl_code)) {
        TR_ERR("FATAL ERROR! OCCURRED: %s", MblError_to_str(mbl_code));
        mbed_client_state_.store(State_DeviceUnregistered);
        return;
    }

    if (!is_action_needed_for_error(mbl_code)) {
        TR_DEBUG("Action is not needed for error %s", MblError_to_str(mbl_code));
        return;
    }

    MbedClientDeviceState current_state = mbed_client_state_.load();

    // Client unregister in progress:
    if (State_DeviceUnregisterInProgress == current_state) {
        TR_ERR("Client unregister failed with error: %s", MblError_to_str(mbl_code));
        // We have no choice but to signal that client is unregistered which will close
        // mbed client and unregister ungracefully
        mbed_client_state_.store(State_DeviceUnregistered);
        return;
    }

    // Client register in progress:
    if (State_DeviceRegisterInProgress == current_state) {
        TR_ERR("Client register failed with error: %s", MblError_to_str(mbl_code));
        // We have no choice but to signal that client is unregistered which will close
        // client and unregister ungracefully
        mbed_client_state_.store(State_DeviceUnregistered);
        return;
    }

    // Notify resource broker as we are in one of the following cases:
    // 1. application requested to register resources
    // 2. keep alive (does not occur during #1)
    if (mbed_client_error_callback_func_) {
        mbed_client_error_callback_func_(mbl_code);
    }
}

} // namespace mbl
