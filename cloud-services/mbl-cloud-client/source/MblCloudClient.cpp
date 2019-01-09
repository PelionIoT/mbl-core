/*
 * Copyright (c) 2017 ARM Ltd.
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

#include "MblCloudClient.h"

#include "MblScopedLock.h"
#include "monotonic_time.h"
#include "update_handlers.h"

#include "mbed-trace/mbed_trace.h"
#include "ns-hal-pal/ns_event_loop.h"

#include <cassert>
#include <csignal>
#include <unistd.h>

#include MBED_CLOUD_CLIENT_USER_CONFIG_FILE

#define TRACE_GROUP "mbl"

static volatile sig_atomic_t g_shutdown_signal = 0;

// Period between re-registrations with the LWM2M server.
// MBED_CLOUD_CLIENT_LIFETIME is how long we should stay registered after each
// re-registration
static const int g_reregister_period_s = MBED_CLOUD_CLIENT_LIFETIME / 2;

extern "C" void mbl_shutdown_handler(int signal)
{
    g_shutdown_signal = signal;
}

static void* get_dummy_network_interface()
{
    static uint32_t network = 0xFFFFFFFF;
    return &network;
}

namespace mbl {

MblCloudClient* MblCloudClient::s_instance = 0;
MblMutex MblCloudClient::s_mutex;


MblCloudClient::InstanceScoper::InstanceScoper()
{
    assert(!s_instance);
    s_instance = new MblCloudClient;
}

MblCloudClient::InstanceScoper::~InstanceScoper()
{
    assert(s_instance);
    delete s_instance;
}

MblCloudClient::MblCloudClient()
    : cloud_client_(new MbedCloudClient)
    , state_(State_Unregistered)
{
}

MblCloudClient::~MblCloudClient()
{
    // 1. Set s_instance to 0 so that callbacks no longer try to access this
    // object
    {
        MblScopedLock l(s_mutex);
        assert(s_instance);
        s_instance = 0;
    }

    // 2. Close and delete the MbedCloudClient. This must be done before
    // stopping the mbed event loop, otherwise MbedCloudClient's dtor might
    // wait on a mutex that will never be released by the event loop.
    assert(cloud_client_);
    cloud_client_->close();
    delete cloud_client_;

    // 3. Stop the mbed event loop thread (which was started in
    // MbedCloudClient's ctor).
    ns_event_loop_thread_stop();
}

MblError MblCloudClient::run()
{
    InstanceScoper scoper;
    assert(s_instance);

    s_instance->register_handlers();
    s_instance->add_resources();

    const MblError ccs_err = s_instance->cloud_client_setup();
    if (ccs_err != Error::None) {
        return ccs_err;
    }

    const MblError ccrb_init = s_instance->cloud_connect_resource_broker_.Init();
    if(0 != ccrb_init) {
        tr_error("Init cloud_connect_resource_broker_ failed with error %s", MblError_to_str(ccrb_init));
    }

    time_t next_registration_s = get_monotonic_time_s() + g_reregister_period_s;
    for (;;) {
        if (g_shutdown_signal) {
            tr_warn("Recieved signal \"%s\", shutting down", strsignal(g_shutdown_signal));
            return Error::ShutdownRequested;
        }

        {
            MblScopedLock l(s_mutex);
            if (s_instance->state_ == State_Unregistered) {
                return Error::DeviceUnregistered;
            }
        }

        const time_t time_s = get_monotonic_time_s();
        if (time_s >= next_registration_s) {
            tr_debug("Updating registration with LWM2M server");
            s_instance->cloud_client_->register_update();
            next_registration_s = time_s + g_reregister_period_s;
        }

        sleep(1);
    }

    return Error::Unknown;
}

void MblCloudClient::register_handlers()
{
    cloud_client_->on_registered(&MblCloudClient::handle_client_registered);
    cloud_client_->on_unregistered(&MblCloudClient::handle_client_unregistered);
    cloud_client_->on_error(&MblCloudClient::handle_error);
    cloud_client_->set_update_progress_handler(&update_handlers::handle_download_progress);
    cloud_client_->set_update_authorize_handler(&handle_authorize);
}

void MblCloudClient::add_resources()
{
    M2MObjectList objs;
    cloud_client_->add_objects(objs);
}

MblError MblCloudClient::cloud_client_setup()
{
    {
        MblScopedLock l(s_mutex);
        state_ = State_CalledRegister;
    }

    const bool setup_ok = cloud_client_->setup(get_dummy_network_interface());
    if (!setup_ok) {
        tr_err("Client setup failed");
        return Error::ConnectUnknownError;
    }

    return Error::None;
}

void MblCloudClient::handle_client_registered()
{
    // Called by the mbed event loop - *s_instance can be destroyed whenever
    // s_mutex isn't locked.

    tr_info("Client registered");

    MblScopedLock l(s_mutex);

    if (!s_instance) {
        return;
    }

    s_instance->state_ = State_Registered;

    const ConnectorClientEndpointInfo* const endpoint = s_instance->cloud_client_->endpoint_info();
    if (endpoint) {
        tr_info("Endpoint Name: %s", endpoint->endpoint_name.c_str());
        tr_info("Device Id: %s", endpoint->internal_endpoint_name.c_str());
    }
    else {
        tr_warn("Failed to get endpoint info");
    }
}

void MblCloudClient::handle_client_unregistered()
{
    // Called by the mbed event loop - *s_instance can be destroyed whenever
    // s_mutex isn't locked.

    {
        MblScopedLock l(s_mutex);
        if (!s_instance) {
            return;
        }
        s_instance->state_ = State_Unregistered;
    }
    tr_warn("Client unregistered");
}

void MblCloudClient::handle_error(const int cloud_client_code)
{
    // Called by the mbed event loop - *s_instance can be destroyed whenever
    // s_mutex isn't locked.

    const MblError mbl_code = CloudClientError_to_MblError(static_cast<MbedCloudClient::Error>(cloud_client_code));
    tr_err("Error occurred : %s", MblError_to_str(mbl_code));
    tr_err("Error code : %d", mbl_code);

    MblScopedLock l(s_mutex);
    if (s_instance) {
        tr_err("Error details : %s",s_instance->cloud_client_->error_description());
    }
    else {
        tr_err("Error details : Failed to obtain error description");
    }
}

void MblCloudClient::handle_authorize(const int32_t request)
{
    // Called by the mbed event loop - *s_instance can be destroyed whenever
    // s_mutex isn't locked.

    if (update_handlers::handle_authorize(request)) {
        MblScopedLock l(s_mutex);

        if (s_instance) {
            s_instance->cloud_client_->update_authorize(request);
        }
    }
}

} // namespace mbl
