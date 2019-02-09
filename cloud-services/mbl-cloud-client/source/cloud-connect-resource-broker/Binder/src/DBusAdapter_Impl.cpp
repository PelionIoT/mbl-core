/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cassert>

#include "DBusAdapter.h"
#include "DBusAdapterMailbox.h"
#include "DBusMailboxMsg.h"
#include "DBusAdapterService.h"
#include "DBusAdapter_Impl.h"


namespace mbl {

DBusAdapter::DBusAdapterImpl::DBusAdapterImpl()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

DBusAdapter::DBusAdapterImpl::~DBusAdapterImpl()
{
    tr_debug("%s", __PRETTY_FUNCTION__);   
};


MblError DBusAdapter::DBusAdapterImpl::bus_init()
{
    tr_debug("%s", __PRETTY_FUNCTION__);    
    int r = -1;
    
    if (nullptr == event_loop_handle_){
        return MblError::DBusErr_Temporary;
    }   
    r = sd_bus_open_user(&connection_handle_);    
    if (r < 0){
        return MblError::DBusErr_Temporary;
    }
    if (nullptr == connection_handle_){
        return MblError::DBusErr_Temporary;
    }    
    r = sd_bus_attach_event(connection_handle_, event_loop_handle_, SD_EVENT_PRIORITY_NORMAL);
    if (r < 0){
        return MblError::DBusErr_Temporary;
    }

    const sd_bus_vtable* table = DBusAdapterService_get_service_vtable();
    if (nullptr == table){
        return MblError::DBusErr_Temporary;
    }
    // Install the object
    r = sd_bus_add_object_vtable(connection_handle_,
                                 nullptr,
                                 DBUS_CLOUD_CONNECT_OBJECT_PATH,  
                                 DBUS_CLOUD_CONNECT_INTERFACE_NAME,
                                 DBusAdapterService_get_service_vtable(),
                                 this);
    if (r < 0) {
        return MblError::DBusErr_Temporary;
    }

    r = sd_bus_get_unique_name(connection_handle_, &unique_name_);
    if (r < 0) {
        return MblError::DBusErr_Temporary;
    }

    //     // Take a well-known service name DBUS_CLOUD_SERVICE_NAME so client Apps can find us
    r = sd_bus_request_name(connection_handle_, DBUS_CLOUD_SERVICE_NAME, 0);
    if (r < 0) {
        return MblError::DBusErr_Temporary;
    }
    service_name_ = DBUS_CLOUD_SERVICE_NAME;
    
    // match signal NameOwnerChanged
    // This signal indicates that the owner of a name has changed. 
    // It's also the signal to use to detect the appearance of new names on the bus.
    r = sd_bus_add_match(
        connection_handle_,
        NULL, 
        "type='signal',interface='org.freedesktop.DBus',member='NameOwnerChanged'",
        DBusAdapter::DBusAdapterImpl::name_owner_changed_match_callback, 
        this);
    if (r < 0) {
        return MblError::DBusErr_Temporary;
    }

    // match signal NameLost
    // This signal is sent to a specific application when it loses ownership of a name.
    r = sd_bus_add_match(
        connection_handle_,
        NULL, 
        "type='signal',interface='org.freedesktop.DBus',member='NameLost'",
        DBusAdapter::DBusAdapterImpl::name_owner_changed_match_callback, 
        this);
    if (r < 0) {
        return MblError::DBusErr_Temporary;
    }

    // match signal NameAcquired
    // This signal is sent to a specific application when it gains ownership of a name.
    r = sd_bus_add_match(
        connection_handle_,
        NULL, 
        "type='signal',interface='org.freedesktop.DBus',member='NameAcquired'",
        DBusAdapter::DBusAdapterImpl::name_owner_changed_match_callback, 
        this);
    if (r < 0) {
        return MblError::DBusErr_Temporary;
    }
   
    return MblError::None;
}

MblError DBusAdapter::DBusAdapterImpl::bus_deinit()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    int r = 0;
    
    if (connection_handle_){

        if (service_name_){
            r = sd_bus_release_name(connection_handle_, DBUS_CLOUD_SERVICE_NAME);
            if (r < 0){
                // TODO : print error
            }
            else {
                service_name_ = nullptr;
            }        
        }
        r = sd_bus_detach_event(connection_handle_);
        if (r < 0){
            // TODO : print error
        }
        connection_handle_ = sd_bus_flush_close_unref(connection_handle_);
    }
   
    return (r < 0) ? MblError::DBusErr_Temporary : MblError::None;
}

MblError DBusAdapter::DBusAdapterImpl::event_loop_init()
{    
    tr_debug("%s", __PRETTY_FUNCTION__);

    //sd_event *handle = NULL;
    int r = 0;
    
    r = sd_event_default(&event_loop_handle_);
    if (r < 0){
        return MblError::DBusErr_Temporary;
    }

    r = sd_event_add_io(
        event_loop_handle_, 
        nullptr,    //event source will be destroyed with the event loop
        mailbox_.get_pipefd_read(), 
        EPOLLIN, 
        DBusAdapter::DBusAdapterImpl::incoming_mailbox_message_callback,
        this);
    if (r < 0){
        return MblError::DBusErr_Temporary;
    }

    return MblError::None;
}

MblError DBusAdapter::DBusAdapterImpl::event_loop_deinit()
{    
    tr_debug("%s", __PRETTY_FUNCTION__);
    
    if (event_loop_handle_){       
        sd_event_unref(event_loop_handle_);
        event_loop_handle_ = nullptr;
    }
    return MblError::None;
}


MblError DBusAdapter::DBusAdapterImpl::event_loop_request_stop(MblError stop_status) 
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    int r;

    //only my thread id is allowd to call this one, check no other thread
    //is using this function in order to prevent confusion.
    //any other thread should send a DBUS_ADAPTER_MSG_EXIT message via mailbox
    if (pthread_equal(pthread_self(), initializer_thread_id_) == 0){
        // current thread id !=  initializer_thread_id_
        return MblError::DBusErr_Temporary;
    }
    r = sd_event_exit(event_loop_handle_, (int)stop_status);
    return (r < 0) ? MblError::DBusErr_Temporary : MblError::None;
}

int DBusAdapter::DBusAdapterImpl::name_owner_changed_match_callback_impl(
    sd_bus_message *m, sd_bus_error *ret_error)
{
    // This signal indicates that the owner of a name has changed. 
    // It's also the signal to use to detect the appearance of new names on the bus.
    // Argument	    Type	Description
    // 0	        STRING	Name with a new owner
    // 1	        STRING	Old owner or empty string if none   
    // 2	        STRING	New owner or empty string if none
    const char *msg_args[3];
    int r = sd_bus_message_read(m, "sss", &msg_args[0], &msg_args[1], &msg_args[2]);
    // for now, we do nothing with this - only print to log
    // TODO : print to log
    return (r < 0) ? r : 0; //TODO - need to understand the meaning when callback return error for all cases
}


int DBusAdapter::DBusAdapterImpl::name_owner_changed_match_callback(
    sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    DBusAdapter::DBusAdapterImpl *adapter_impl = static_cast<DBusAdapter::DBusAdapterImpl*>(userdata);
    return adapter_impl->name_owner_changed_match_callback_impl(m, ret_error);   
}

int  DBusAdapter::DBusAdapterImpl::incoming_mailbox_message_callback(
    sd_event_source *s, 
    int fd,
 	uint32_t revents,
    void *userdata)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    DBusAdapter::DBusAdapterImpl *adapter_impl = static_cast<DBusAdapter::DBusAdapterImpl*>(userdata);
    return adapter_impl->incoming_mailbox_message_callback_impl(s, fd, revents);
}

int DBusAdapter::DBusAdapterImpl::incoming_mailbox_message_callback_impl(
    sd_event_source *s, 
    int fd,
 	uint32_t revents)
{
    MblError status;
    DBusMailboxMsg msg;
    int r = -1;

    if ((revents & EPOLLIN) == 0){
         // TODO : print , fatal error. what to do?
        return -1;
    }
    if (fd != mailbox_.get_pipefd_read()){
        return r;
    }
    status = mailbox_.receive_msg(
        msg, 
        DBusAdapterMailbox::DBUS_MAILBOX_MAX_DEFAULT_TIMEOUT_MILLISECONDS);
    if (status != MblError::None) {
        return r;
    }

    switch (msg.type_)
    {
        case mbl::DBusMailboxMsg::MsgType::EXIT:
            if (msg.payload_len_ != sizeof(mbl::DBusMailboxMsg::Msg_exit_)){
                break;
            }
            r = event_loop_request_stop(msg.payload_.exit.stop_status);
            break;
        case mbl::DBusMailboxMsg::MsgType::RAW_DATA:
            //pass 
            break;
    
        default:
            //TODO : print error
            break;
    }
    return r;
}

int DBusAdapter::DBusAdapterImpl::incoming_bus_message_callback_impl(
    sd_bus_message *m, sd_bus_error *ret_error)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    int r;
    bool increase_reference = false;


    //TODO: check what happens when failure returned to callback
    if (sd_bus_message_is_empty(m)){
        return -1;
    }
    if (0 != strncmp(
        sd_bus_message_get_destination(m),
        DBUS_CLOUD_SERVICE_NAME, 
        strlen(DBUS_CLOUD_SERVICE_NAME)))
    {
        return -1;        
    }
    if (0 != strncmp(
        sd_bus_message_get_path(m),
        DBUS_CLOUD_CONNECT_OBJECT_PATH, 
        strlen(DBUS_CLOUD_CONNECT_OBJECT_PATH)))
    {
        return -1;        
    }
    if (0 != strncmp(
        sd_bus_message_get_interface(m),
        DBUS_CLOUD_CONNECT_INTERFACE_NAME, 
        strlen(DBUS_CLOUD_CONNECT_INTERFACE_NAME)))
    {
        return -1;        
    }

    if (sd_bus_message_is_method_call(m, 0, "RegisterResources")){
        const char *json_file_data = NULL;       
        
        if (sd_bus_message_has_signature(m, "s") == false){
            return -1;        
        }
        r = sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &json_file_data);
        if (r < 0){
            return r;
        }

        if ((NULL == json_file_data) || (strlen(json_file_data) == 0)){
            return -1;
        }

        // TODO:
        // validate app registered expected interface on bus? (use sd-bus track)
        
        r = process_incoming_message_RegisterResources(m, json_file_data);
        if (r < 0){
            //TODO : print , fatal error. what to do?
            return r;
        }
        // success - increase ref
        sd_bus_message_ref(m);
    }
    else if (sd_bus_message_is_method_call(m, 0, "DeregisterResources")) {
        const char *access_token = NULL;       
        
        if (sd_bus_message_has_signature(m, "s") == false){
            return -1;        
        }
        r = sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &access_token);
        if (r < 0){
            return r;
        }

        if ((NULL == access_token) || (strlen(access_token) == 0)){
            return -1;
        }

        r = process_incoming_message_DeregisterResources(m, access_token);
        if (r < 0){
            //TODO : print , fatal error. what to do?
            return r;
        }
        // success - increase ref 
        sd_bus_message_ref(m);
    }
    else {
        //TODO - reply with error? or just pass?
        return -1;
    }

    return 0;
}

int DBusAdapter::DBusAdapterImpl::process_incoming_message_RegisterResources(
    const sd_bus_message *m, 
    const char *appl_resource_definition_json)
{
    // Register resources is an asynchronous process towards the cloud -> store handle
    if (pending_messages_.insert(m).second == false){
        return -1;
    }
    //TODO : uncomment + handle errors
    //MblError status = ccrb.register_resources(bus_request_handle, std::string(appl_resource_definition_json));    
    return 0;
}

int DBusAdapter::DBusAdapterImpl::process_incoming_message_DeregisterResources(
    const sd_bus_message *m, 
    const char *access_toke) 
{
    // Register resources is an asynchronous process towards the cloud -> store handle
    if (pending_messages_.insert(m).second == false){
        return -1;
    }
    //TODO : uncomment + handle errors
    //MblError status = ccrb.deregister_resources(bus_request_handle, std::string(deregister_resources));    
    return 0;
}

MblError DBusAdapter::DBusAdapterImpl::init()
{
    tr_debug("%s", __PRETTY_FUNCTION__);    
    MblError status;
    int r;

    if (State::UNINITALIZED != state_){
        return Error::DBusErr_Temporary;
    }

    status = mailbox_.init();
    if (status != Error::None){
        //mailbox deinit itself
        return status;
    }

    status = event_loop_init();
    if (status != Error::None){
        //event loop is not an object, need to deinit
        event_loop_deinit();
        return status;
    }

    status = bus_init();
    if (status != Error::None){
        //bus is not an object, need to deinit
        bus_deinit();    
        return status;
    }

    initializer_thread_id_ = pthread_self();
    state_ = State::INITALIZED;
    return Error::None;
}

MblError DBusAdapter::DBusAdapterImpl::deinit()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    MblError status1, status2, status3;
    int r;

    if (State::INITALIZED != state_){
        return Error::DBusErr_Temporary;
    }
            
    status1 = mailbox_.deinit();
    status2 = bus_deinit();
    status3 = event_loop_deinit();

    // TODO - clean all references to messages, timers etc. to unref all
    //for (auto &handle : bus_request_handles_){
        //sd_bus_message_unref((sd_bus_message*)handle);
    //}

    if (MblError::None != status1) {
        return status1;
    }
    else if (MblError::None != status2) {
        return status2;
    }
    else if (MblError::None != status3) {
        return status3;
    }
    state_ = State::UNINITALIZED;
    return Error::None;
}


MblError DBusAdapter::DBusAdapterImpl::event_loop_run(MblError &stop_status) 
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    /*
    Thread enters the sd-event loop and blocks.
    https://www.freedesktop.org/software/systemd/man/sd_event_run.html# :
    sd_event_loop() invokes sd_event_run() in a loop, 
    thus implementing the actual event loop. The call returns as soon as exiting 
    was requested using sd_event_exit(3).
    sd_event_loop() returns the exit code specified when invoking sd_event_exit()
    */
    state_ = State::RUNNING;
    stop_status = (MblError)sd_event_loop(event_loop_handle_);
    state_ = State::INITALIZED;
    return MblError::None;
}


MblError DBusAdapter::DBusAdapterImpl::run(MblError &stop_status)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    MblError status;
    int r;

    if (State::INITALIZED != state_){
        return Error::DBusErr_Temporary;
    }
        
    status = event_loop_run(stop_status);    
    if (status != MblError::None){
        event_loop_request_stop(MblError::DBusStopStatusErrorInternal);
        return status;
    }
    state_ = State::INITALIZED;

    return MblError::None;
}


MblError DBusAdapter::DBusAdapterImpl::stop(MblError stop_status)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    MblError status;
    //TODO : need to understand by design
    int exit_code = 0; //set 0 as exit code for now - we don't pass the exit code from outside
    if (State::UNINITALIZED == state_){
        return Error::DBusErr_Temporary;
    }

    if (pthread_equal(pthread_self(), initializer_thread_id_) != 0){
        // current thread id ==  initializer_thread_id_
        int r = event_loop_request_stop(stop_status);
        if (r < 0){
            // print error
        }
        status = (r >= 0) ? Error::None : Error::DBusErr_Temporary;
    }
    else {
        DBusMailboxMsg msg;
        msg.type_ = mbl::DBusMailboxMsg::MsgType::EXIT;
        msg.payload_len_ = sizeof(mbl::DBusMailboxMsg::Msg_exit_);
        msg.payload_.exit.stop_status = stop_status;
        status = mailbox_.send_msg(msg, MSG_SEND_ASYNC_TIMEOUT_MILLISECONDS);
        if (status != MblError::None){
            //print something
        }
    }
    return status;
}

MblError DBusAdapter::DBusAdapterImpl::handle_ccrb_RegisterResources_status_update(
    const uintptr_t ipc_conn_handle, 
    const std::string &access_token,
    const CloudConnectStatus reg_status)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(0);

    if (State::RUNNING != state_){
        return Error::DBusErr_Temporary;
    }
}

MblError DBusAdapter::DBusAdapterImpl::handle_ccrb_DeregisterResources_status_update(
    const uintptr_t ipc_conn_handle, 
    const CloudConnectStatus dereg_status)
{
    tr_debug("%s", __PRETTY_FUNCTION__)
    assert(0);

    if (State::RUNNING != state_){
        return Error::DBusErr_Temporary;
    }
}

MblError DBusAdapter::DBusAdapterImpl::handle_ccrb_AddResourceInstances_status_update(
    const uintptr_t ipc_conn_handle, 
    const CloudConnectStatus add_status)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(0);

    if (State::RUNNING != state_){
        return Error::DBusErr_Temporary;
    }   
}

MblError DBusAdapter::DBusAdapterImpl::handle_ccrb_RemoveResourceInstances_status_update(
    const uintptr_t ipc_conn_handle, 
    const CloudConnectStatus remove_status)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(0);

    if (State::RUNNING != state_){
        return Error::DBusErr_Temporary;
    }
    
}
}