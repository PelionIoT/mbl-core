/*
 * Copyright (c) 2016-2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: ...
 */

#include <cassert>
#include <pthread.h>
#include <unistd.h>

//#include "mbed-trace/mbed_trace.h"  // FIXME : uncomment
#include "DBusAdapter.h"
#include "DBusAdapterLowLevel.h"

#define TRACE_GROUP "ccrb-dbus"


namespace mbl {

//TODO  uncomment and solve gtest issue for unit tests
/*
DBusAdapter::DBusAdapter(ResourceBroker &ccrb) :
    ccrb_(ccrb)
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    // set callbacks to DBUS lower layer    
    lower_level_callbacks_.register_resources_async_callback = register_resources_async_callback;
    lower_level_callbacks_.deregister_resources_async_callback = deregister_resources_async_callback;
}
*/

//TODO  delete - temporary ctor
DBusAdapter::DBusAdapter()
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    // set callbacks from D-BUS service from lower layer    
    lower_level_callbacks_.register_resources_async_callback = register_resources_async_callback;
    lower_level_callbacks_.deregister_resources_async_callback = deregister_resources_async_callback;

    // set other callbacks from lower layer
    lower_level_callbacks_.received_message_on_mailbox_callback = received_message_on_mailbox_callback;
}

DBusAdapter::~DBusAdapter()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

MblError DBusAdapter::init()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    MblError status;
    int r;

    if (DBusAdapter::Status::NON_INITALIZED != status_){
        return Error::DBusErr_Temporary;
    }
    status = mailbox_.init();
    if (status != Error::None){
        deinit();
        return status;
    }

    r = DBusAdapterLowLevel_init(&lower_level_callbacks_);
    if (r < 0){
        deinit();
        return Error::DBusErr_Temporary;
    }

    status = mailbox_.add_read_fd_to_event_loop();
    if (status != Error::None){
        deinit();
        return status;
    }

    status_ = DBusAdapter::Status::INITALIZED;
    return Error::None;
}

MblError DBusAdapter::deinit()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    MblError status;
    int r;

    if (DBusAdapter::Status::INITALIZED != status_){
        return Error::DBusErr_Temporary;
    }
    
    status = mailbox_.deinit();
    if (status != Error::None){
        return status;
    }

    r = DBusAdapterLowLevel_deinit();
    if (r < 0){
        return Error::DBusErr_Temporary;
    }

    status_ = DBusAdapter::Status::NON_INITALIZED;
    return Error::None;
}

MblError DBusAdapter::run()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(0);
    if (DBusAdapter::Status::INITALIZED != status_){
        return Error::DBusErr_Temporary;
    }
    
    return Error::None;
}

MblError DBusAdapter::stop()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(0);
    if (DBusAdapter::Status::INITALIZED != status_){
        return Error::DBusErr_Temporary;
    }
    return Error::None;
}


MblError DBusAdapter::update_registration_status(
    const uintptr_t , 
    const std::string &,
    const CloudConnectStatus )
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(0);
    if (DBusAdapter::Status::INITALIZED != status_){
        return Error::DBusErr_Temporary;
    }
    return Error::None;
}

MblError DBusAdapter::update_deregistration_status(
    const uintptr_t , 
    const CloudConnectStatus )
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(0);
    if (DBusAdapter::Status::INITALIZED != status_){
        return Error::DBusErr_Temporary;
    }
    return Error::None;
}

MblError DBusAdapter::update_add_resource_instance_status(
    const uintptr_t , 
    const CloudConnectStatus )
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(0);
    if (DBusAdapter::Status::INITALIZED != status_){
        return Error::DBusErr_Temporary;
    }
    return Error::None;
}

MblError DBusAdapter::update_remove_resource_instance_status(
    const uintptr_t , 
    const CloudConnectStatus )
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(0);
    if (DBusAdapter::Status::INITALIZED != status_){
        return Error::DBusErr_Temporary;
    }
    return Error::None;
}



int DBusAdapter::register_resources_async_callback(
        const uintptr_t ipc_conn_handle, 
        const char *appl_resource_definition_json)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(0);
    return 0;
}

int DBusAdapter::deregister_resources_async_callback(
    const uintptr_t ipc_conn_handle, 
    const char *access_token)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(0);
    return 0;
}

int DBusAdapter::received_message_on_mailbox_callback(
    const uint8_t* buff, 
    const uint32_t size)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(0);
    return 0;
}

} // namespace mbl
