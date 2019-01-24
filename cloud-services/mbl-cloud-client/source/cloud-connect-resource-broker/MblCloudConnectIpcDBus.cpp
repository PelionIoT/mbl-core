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

#include <systemd/sd-bus.h>

#define TRACE_GROUP "ccrb-dbus"

namespace mbl {

MblCloudConnectIpcDBus::MblCloudConnectIpcDBus()
    : exit_loop_ (false) // temporary flag exit_loop_ will be removed soon
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

MblCloudConnectIpcDBus::~MblCloudConnectIpcDBus()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

MblError MblCloudConnectIpcDBus::init()
{
    tr_info("%s", __PRETTY_FUNCTION__);
    return Error::None;
}

MblError MblCloudConnectIpcDBus::de_init()
{
    tr_info("%s", __PRETTY_FUNCTION__);
    return Error::None;
}

MblError MblCloudConnectIpcDBus::run()
{

    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *m = NULL;
    sd_bus *bus = NULL;
    const char *path;
    
    /* Connect to the system bus */
    sd_bus_open_system(&bus);
    


    tr_info("%s", __PRETTY_FUNCTION__);
    
    // now we use simulated event-loop that will be removed after we introduce real sd-bus event-loop.
    while(!exit_loop_) {
        sleep(1);
    }

    tr_info("%s: event loop is finished", __PRETTY_FUNCTION__);

    return Error::None;
}

MblError MblCloudConnectIpcDBus::stop()
{
    tr_info("%s", __PRETTY_FUNCTION__);

    // temporary not thread safe solution that should be removed soon.
    // signal to event-loop that it should finish.
    exit_loop_ = true;

    return Error::None;
}

} // namespace mbl

