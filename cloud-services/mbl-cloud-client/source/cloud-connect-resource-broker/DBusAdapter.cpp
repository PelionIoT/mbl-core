/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cassert>
#include <pthread.h>
#include <unistd.h>
#include <systemd/sd-bus.h>

#include "mbed-trace/mbed_trace.h"
#include "DBusAdapter.h"
#include "ResourceBroker.h"

#define TRACE_GROUP "ccrb-dbus"

namespace mbl {

DBusAdapter::DBusAdapter(ResourceBroker &ccrb)
    : exit_loop_(false), // temporary flag exit_loop_ will be removed soon
      ccrb_(ccrb)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

DBusAdapter::~DBusAdapter()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

MblError DBusAdapter::init()
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    // FIXME: temporary - remove code bellow
    sd_bus *bus = NULL;
    int bus_open_status = sd_bus_open_system(&bus);
    tr_info("sd_bus_open_system returned %d", bus_open_status);
    // FIXME: temporary - remove code above

    return Error::None;
}

MblError DBusAdapter::de_init()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    return Error::None;
}

MblError DBusAdapter::run()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    // now we use simulated event-loop that will be removed after we introduce real sd-bus event-loop.
    while(!exit_loop_) {
        sleep(1); // 1 seconds
    }

    tr_info("%s: event loop is finished", __PRETTY_FUNCTION__);

    return Error::None;
}

MblError DBusAdapter::stop()
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    // temporary not thread safe solution that should be removed soon.
    // signal to event-loop that it should finish.
    exit_loop_ = true;

    return Error::None;
}


MblError DBusAdapter::update_registration_status(
    const uintptr_t /*unused*/, 
    const CloudConnectStatus /*unused*/)
{
    tr_debug("%s", __PRETTY_FUNCTION__);    
    // empty for now
    return Error::None;
}

MblError DBusAdapter::update_deregistration_status(
    const uintptr_t /*unused*/, 
    const CloudConnectStatus /*unused*/)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    // empty for now
    return Error::None;
}

MblError DBusAdapter::update_add_resource_instance_status(
    const uintptr_t /*unused*/, 
    const CloudConnectStatus /*unused*/)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    // empty for now
    return Error::None;
}

MblError DBusAdapter::update_remove_resource_instance_status(
    const uintptr_t /*unused*/, 
    const CloudConnectStatus /*unused*/)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    // empty for now
    return Error::None;
}

} // namespace mbl
