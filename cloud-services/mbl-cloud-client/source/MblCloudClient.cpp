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
#include "update_handlers.h"

#include "mbed-trace/mbed_trace.h"
#include "ns-hal-pal/ns_event_loop.h"

#include <cassert>
#include <csignal>
#include <unistd.h>

#define TRACE_GROUP "mbl"

static volatile sig_atomic_t g_shutdown_signal = 0;

extern "C" void mbl_shutdown_handler(int signal)
{
    g_shutdown_signal = signal;
}

namespace mbl
{

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

MblCloudClient::MblCloudClient() : cloud_client_(new MbedCloudClient), state_(State_Unregistered)
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
    tr_info("~MblCloudClient close mbed client");
    assert(cloud_client_);
    cloud_client_->close();
    delete cloud_client_;

    // 3. Stop the mbed event loop thread (which was started in
    // MbedCloudClient's ctor).
    tr_info("~MblCloudClient Stop the mbed event loop thread");
    ns_event_loop_thread_stop();
}

MblError MblCloudClient::run()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    InstanceScoper scoper;
    assert(s_instance);

    s_instance->register_handlers();

    // start running CCRB module
    const MblError ccrb_start_err =
        s_instance->cloud_connect_resource_broker_.start(s_instance->cloud_client_);
    if (Error::None != ccrb_start_err) {
        tr_err("CCRB module start() failed! (%s)", MblError_to_str(ccrb_start_err));
        return ccrb_start_err;
    } else {
        MblScopedLock l(s_mutex);
        s_instance->state_ = State_CalledRegister;
    }

    for (;;) {
        if (g_shutdown_signal) {
            tr_warn("Received signal \"%s\", shutting down", strsignal(g_shutdown_signal));

            // stop running CCRB module
            const MblError ccrb_stop_err = s_instance->cloud_connect_resource_broker_.stop();
            if (Error::None != ccrb_stop_err) {
                tr_err("CCRB module stop() failed! (%s)", MblError_to_str(ccrb_stop_err));
                return ccrb_stop_err;
            }

            return Error::ShutdownRequested;
        }

        {
            MblScopedLock l(s_mutex);
            if (s_instance->state_ == State_Unregistered) {
                return Error::DeviceUnregistered;
            }
        }

        sleep(1);
    }

    return Error::Unknown;
}

void MblCloudClient::register_handlers()
{
    cloud_client_->set_update_progress_handler(&update_handlers::handle_download_progress);
    cloud_client_->set_update_authorize_handler(&handle_authorize);
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

    const MblError mbl_code =
        CloudClientError_to_MblError(static_cast<MbedCloudClient::Error>(cloud_client_code));
    tr_err("Error occurred : %s", MblError_to_str(mbl_code));
    tr_err("Error code : %d", mbl_code);

    MblScopedLock l(s_mutex);
    if (s_instance) {
        tr_err("Error details : %s", s_instance->cloud_client_->error_description());
    }
    else
    {
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
