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

#include "MblCloudConnectIpcDBus.h"

#include "mbed-trace/mbed_trace.h"
#include <cassert>
#include <pthread.h>
#include <unistd.h>

#define TRACE_GROUP "ccrb-dbus"

namespace mbl {

MblCloudConnectIpcDBus::MblCloudConnectIpcDBus()
    : exit_loop (false) // temporary flag exit_loop will be removed soon
{
    tr_info("MblCloudConnectIpcDBus::MblCloudConnectIpcDBus");

    // constructor runs on IPC thread context
    // store thread ID, always succeeding function
    ipc_thread_id = pthread_self();
}

MblCloudConnectIpcDBus::~MblCloudConnectIpcDBus()
{
    tr_info("MblCloudConnectIpcDBus::~MblCloudConnectIpcDBus");
}

MblError MblCloudConnectIpcDBus::init()
{
    tr_info("MblCloudConnectIpcDBus::init");
    return Error::None;
}

MblError MblCloudConnectIpcDBus::de_init()
{
    tr_info("MblCloudConnectIpcDBus::de_init");
    return Error::None;
}

MblError MblCloudConnectIpcDBus::run()
{
    tr_info("MblCloudConnectIpcDBus::run");
    
    // now we use simulated event-loop that will be removed after we introduce real sd-bus event-loop.
    while(!exit_loop)
    {
        tr_info("event loop is alive");
        sleep(1);
    }

    return Error::None;
}

MblError MblCloudConnectIpcDBus::stop()
{
    tr_info("MblCloudConnectIpcDBus::stop");
    
    MblError stop_ipc_err = stop_event_loop();
    if(Error::None != stop_ipc_err){
        tr_err("Sending finish signal to IPC thread failed! (%s)", MblError_to_str(stop_ipc_err));
        return stop_ipc_err;
    }

    return stop_ipc_err;
}

MblError MblCloudConnectIpcDBus::stop_event_loop()
{
    tr_info("MblCloudConnectIpcDBus::thread_join");

    // temporary not thread safe solution that should be removed soon
    exit_loop = true;

    const int thread_join_err = pthread_join(ipc_thread_id, nullptr);
    if(0 != thread_join_err)
    {
        // thread joining failed, print errno value and exit
        const int thread_join_errno = errno;

        tr_err(
            "Thread joining failed (%s)!\n",
            strerror(thread_join_errno));

        return Error::CCRBStoppingFailed;
    }

    return Error::None;
}

} // namespace mbl

