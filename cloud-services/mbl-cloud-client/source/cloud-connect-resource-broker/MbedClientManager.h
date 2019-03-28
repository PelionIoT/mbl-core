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


#ifndef MbedClientManager_h_
#define MbedClientManager_h_

#include "MblError.h"
#include "update_handlers.h"

#include <functional>
#include <atomic>

namespace mbl {

class ResourceBroker;

class MbedClientManager {

public:

    MbedClientManager(ResourceBroker& ccrb);
    virtual ~MbedClientManager();

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
    virtual MblError init_mbed_client();

    /**
     * @brief Deinit mbed client
     * Called after unregister client is finished as part of deinit operation
     * Erase mbed client that was created in init_mbed_client()
     * Stop the mbed event loop thread so no callbacks will be fired
     */
    virtual void deinit_mbed_client();

    /**
     * @brief Unregister mbed client
     * This will set an atomic enum state to signal main loop that unregister is started
     * Unregister operation ends when a callback is called by mbed client.
     * This function is called when terminate signal arrive.
     */
    virtual void unregister_mbed_client();

    /**
     * @brief Return true if device is registered
     * 
     * @return true if device is registered
     * @return false if device is not registered (e.g. registration in progress is NOT registered)
     */
    virtual bool is_device_registered();

    /**
     * @brief Return true if device is unregistered
     * 
     * @return true if device is unregistered
     * @return false if device is not unregistered 
     * (e.g. un-registration in progress is NOT registered)
     */
    virtual bool is_device_unregistered();

    /**
     * @brief Perform keepalive to mbed client
     * 
     */
    virtual void keepalive();

    /**
     * @brief Register resources
     * 
     * @param object_list - resources to be registered
     */
    virtual void register_resources(const M2MObjectList& object_list);

protected:

    // Mbed client (device) states
    // These states represent the devices registration states against Pelion
    // Only when Mbed client (device) state is registered - an application can register
    // its own resources using resource broker APIs
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

private:

    // pointer to a static MbedClientManager instance to be used by callbacks
    // initialized in MbedClientManager c'tor
    static MbedClientManager* s_instance;

    // Reference to the Cloud Conect Resource Broker (CCRB) which acts as the upper layer
    ResourceBroker& ccrb_;    

    // Mbed cloud client
    MbedCloudClient* cloud_client_;

    // Dummy network interface needed by cloud_client_->setup API (used only in MbedOS)
    static uint32_t dummy_network_interface_;

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
     * @brief - Registration update callback.
     * Called by Mbed cloud client to indicate last mbed-cloud-client registration update was 
     * successful.
     */
    void handle_mbed_client_registration_updated();

    /**
     * @brief Determain if an action is needed based on mbed client error code
     * 
     * @param mbed_client_error - Mbed client error code
     * @return true - in case action is needed
     * @return false - in case action is not needed
     */
    bool is_action_needed_for_error(MblError mbed_client_error);

    /**
     * @brief - Error callback function
     * Called by Mbed Cloud Client to indicate last mbed-cloud-client operation failure
     * 
     * @param cloud_client_code - Mbed cloud client error code for the last register / deregister operations.
     */
    void handle_mbed_client_error(const int cloud_client_code);
};

} // namespace mbl

#endif // MbedClientManager_h_
