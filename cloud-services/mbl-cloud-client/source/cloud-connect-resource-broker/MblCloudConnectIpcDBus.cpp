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

MblCloudConnectIpcDBus::MblCloudConnectIpcDBus(MblCloudConnectResourceBroker &ccrb)
    : exit_loop_ (false), // temporary flag exit_loop_ will be removed soon
      ccrb_ (ccrb)
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

    // FIXME: temporary - remove code bellow
    sd_bus *bus = NULL;
    int bus_open_status = sd_bus_open_system(&bus);
    tr_info("sd_bus_open_system returned %d", bus_open_status);
    // FIXME: temporary - remove code above

    return Error::None;
}

MblError MblCloudConnectIpcDBus::de_init()
{
    tr_info("%s", __PRETTY_FUNCTION__);
    return Error::None;
}

MblError MblCloudConnectIpcDBus::run()
{
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

MblError MblCloudConnectIpcDBus::update_registration_status(
    const uintptr_t , 
    const uintptr_t , 
    const std::string ,
    const MblError )
{
    // empty for now
    return Error::None;
}

MblError MblCloudConnectIpcDBus::update_deregistration_status(
    const uintptr_t , 
    const uintptr_t , 
    const MblError )
{
    // empty for now
    return Error::None;
}

MblError MblCloudConnectIpcDBus::update_add_resource_instance_status(
    const uintptr_t , 
    const uintptr_t , 
    const MblError )
{
    // empty for now
    return Error::None;
}

MblError MblCloudConnectIpcDBus::update_remove_resource_instance_status(
    const uintptr_t , 
    const uintptr_t , 
    const MblError )
{
    // empty for now
    return Error::None;
}

} // namespace mbl

