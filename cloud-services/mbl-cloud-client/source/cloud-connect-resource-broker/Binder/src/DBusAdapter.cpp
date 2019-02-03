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
#include "DBusAdapterMsg.h"

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

    ccrb_thread_id_ = pthread_self();
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
    if (DBusAdapter::Status::INITALIZED != status_){
        return Error::DBusErr_Temporary;
    }
    
    int r = DBusAdapterLowLevel_event_loop_run();    
    return (r == 0) ? Error::None : Error::DBusErr_Temporary;
}

MblError DBusAdapter::stop()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    MblError status;
    int exit_code = 0; //set 0 as exit code for now
    if (DBusAdapter::Status::INITALIZED != status_){
        return Error::DBusErr_Temporary;
    }

    if (pthread_self() == ccrb_thread_id_){
        int r = DBusAdapterLowLevel_event_loop_request_stop(exit_code);
        if (r < 0){
            // print error
        }
        status = (r == 0) ? Error::None : Error::DBusErr_Temporary;
    }
    else
    {
        DBusAdapterMsg msg;
        msg.type = mbl::DBusAdapterMsgType::DBUS_ADAPTER_MSG_EXIT;
        msg.payload_len = sizeof(mbl::DBusAdapterMsg_exit);
        msg.payload.exit.exit_code = exit_code;
        status = mailbox_.send_msg(msg, MSG_SEND_ASYNC_TIMEOUT_MILLISECONDS);
        if (status != MblError::None){
            //print something
        }
    }
    return status;
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
    const int fd,
    const void* userdata)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(0);
    return 0;
}

} // namespace mbl
