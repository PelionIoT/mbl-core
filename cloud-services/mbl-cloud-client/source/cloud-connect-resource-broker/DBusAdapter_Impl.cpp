/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cassert>
#include <vector>
#include <string>

#include "CloudConnectCommon_Internal.h"
#include "DBusAdapter.h"
#include "Mailbox.h"
#include "MailboxMsg.h"
#include "DBusService.h"
#include "DBusAdapter_Impl.h"
#include "ResourceBroker.h"
#include "CloudConnectTypes.h"

#define TRACE_GROUP "ccrb-dbus"

namespace mbl {

DBusAdapterImpl::DBusAdapterImpl(ResourceBroker &ccrb) :
    ccrb_(ccrb)
{
    tr_debug("Enter");
}

MblError DBusAdapterImpl::bus_init()
{
    tr_debug("Enter");    
    int r = -1;
    
    // Enforce initialization of event loop before bus
    if (nullptr == event_loop_handle_){
        tr_error("event_loop_handle_ not initialized! returning %s",
            MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }   

    // Open a connection to the bus. DBUS_SESSION_BUS_ADDRESS should be define as part of the 
    // process environment
    r = sd_bus_open_user(&connection_handle_);    
    if (r < 0){
         tr_error("sd_bus_open_user failed with error r=%d (%s) - returning %s",
            r, strerror(r), MblError_to_str(MblError::DBA_SdBusCallFailure));
        return MblError::DBA_SdBusCallFailure;
    }
    assert(connection_handle_);
    tr_info("D-Bus Connection object created (connection_handle_=%p)", connection_handle_);

    // Attach bus connection object to event loop
    r = sd_bus_attach_event(connection_handle_, event_loop_handle_, SD_EVENT_PRIORITY_NORMAL);
    if (r < 0){
        tr_error("sd_bus_attach_event failed with error r=%d (%s) - returning %s",
            r, strerror(r), MblError_to_str(MblError::DBA_SdBusCallFailure));
        return MblError::DBA_SdBusCallFailure;
    }
    tr_info("Connection object attached to event object");

    // Attach sd-bus vtable Interface DBUS_CLOUD_CONNECT_INTERFACE_NAME under object path 
    // DBUS_CLOUD_CONNECT_OBJECT_PATH to the bus connection
    // The vtable is our publish service
    // userdata for all callbacks - 'this'
    const sd_bus_vtable* service_vtable = DBusService_get_service_vtable();
    assert(service_vtable);
    r = sd_bus_add_object_vtable(connection_handle_,
                                 nullptr,
                                 DBUS_CLOUD_CONNECT_OBJECT_PATH,  
                                 DBUS_CLOUD_CONNECT_INTERFACE_NAME,
                                 service_vtable,
                                 this);
    if (r < 0) {
        tr_error("sd_bus_add_object_vtable failed with error r=%d (%s) - returning %s",
            r, strerror(r), MblError_to_str(MblError::DBA_SdBusCallFailure));
        return MblError::DBA_SdBusCallFailure;
    }
    tr_info("Added new interface %s using service_vtable to object %s",
        DBUS_CLOUD_CONNECT_INTERFACE_NAME,
        DBUS_CLOUD_CONNECT_OBJECT_PATH);

    // Get my unique name on the bus    
    r = sd_bus_get_unique_name(connection_handle_, &unique_name_);
    if (r < 0) {
         tr_error("sd_bus_get_unique_name failed with error r=%d (%s) - returning %s",
            r, strerror(r), MblError_to_str(MblError::DBA_SdBusCallFailure));
        return MblError::DBA_SdBusCallFailure;
    }
    tr_info("unique_name_=%s", unique_name_);
    
    // Request a well-known service name DBUS_CLOUD_SERVICE_NAME so client Apps can find us
    // We do not expect anyone else to already own that name
    r = sd_bus_request_name(connection_handle_, DBUS_CLOUD_SERVICE_NAME, 0);
    if (r < 0) {
        tr_error("sd_bus_request_name failed with error r=%d (%s) - returning %s",
            r, strerror(r), MblError_to_str(MblError::DBA_SdBusRequestNameFailed));
        return MblError::DBA_SdBusRequestNameFailed;
    }
    service_name_ = DBUS_CLOUD_SERVICE_NAME;
    tr_info("Aquired D-Bus known name service_name_=%s", DBUS_CLOUD_SERVICE_NAME);
    
    // TODO - this is a skeleton call - we should conduct more research what to do with it
    // and which one of the next 3 calls we should leave
    // match signal NameOwnerChanged
    // This signal indicates that the owner of a name has changed. 
    // It's also the signal to use to detect the appearance of new names on the bus.
    const char *NameOwnerChanged_match_str = 
        "type='signal',interface='org.freedesktop.DBus',member='NameOwnerChanged'";
    r = sd_bus_add_match(
        connection_handle_,
        NULL, 
        NameOwnerChanged_match_str,
        DBusAdapterImpl::name_changed_match_callback, 
        this);
    if (r < 0) {
        tr_error("sd_bus_add_match failed with error r=%d (%s) - returning %s",
            r, strerror(r), MblError_to_str(MblError::DBA_SdBusRequestAddMatchFailed));
        return MblError::DBA_SdBusRequestAddMatchFailed;
    }
    tr_info("Added D-Bus broker signal match - NameOwnerChanged");

    // TODO - this is a skeleton call - we should conduct more research what to do with it
    // and which one of the next 3 calls we should leave
    // Match signal NameLost
    // This signal is sent to a specific application when it loses ownership of a name.
    r = sd_bus_add_match(
        connection_handle_,
        NULL, 
        "type='signal',interface='org.freedesktop.DBus',member='NameLost'",
        DBusAdapterImpl::name_changed_match_callback, 
        this);
    if (r < 0) {
        tr_error("sd_bus_add_match failed with error r=%d (%s) - returning %s",
            r, strerror(r), MblError_to_str(MblError::DBA_SdBusRequestAddMatchFailed));
        return MblError::DBA_SdBusRequestAddMatchFailed;
    }
    tr_info("Added D-Bus broker signal match - NameLost");

    // TODO - this is a skeleton call - we should conduct more research what to do with it
    // and which one of the next 3 calls we should leave
    // match signal NameAcquired
    // This signal is sent to a specific application when it gains ownership of a name.
    r = sd_bus_add_match(
        connection_handle_,
        NULL, 
        "type='signal',interface='org.freedesktop.DBus',member='NameAcquired'",
        DBusAdapterImpl::name_changed_match_callback, 
        this);
    if (r < 0) {
        tr_error("sd_bus_add_match failed with error r=%d (%s) - returning %s",
            r, strerror(r), MblError_to_str(MblError::DBA_SdBusRequestAddMatchFailed));
        return MblError::DBA_SdBusRequestAddMatchFailed;
    }
    tr_info("Added D-Bus broker signal match - NameAcquired");
   
    return MblError::None;
}

MblError DBusAdapterImpl::bus_deinit()
{
    tr_debug("Enter");
    int r = -1;
    OneSetMblError status;
    
    if (connection_handle_){
        // Release service known name
        if (service_name_){
            r = sd_bus_release_name(connection_handle_, DBUS_CLOUD_SERVICE_NAME);
            if (r < 0){
                tr_error("sd_bus_release_name failed with error r=%d (%s) - returning %s",
                    r1, strerror(r), MblError_to_str(MblError::DBA_SdBusCallFailure));
                status.set(MblError::DBA_SdBusCallFailure);
            }
            else {
                service_name_ = nullptr;
            }        
        }

        // Detach bus connection object from event loop
        r = sd_bus_detach_event(connection_handle_);
        if (r < 0){
            tr_error("sd_bus_detach_event failed with error r=%d (%s) - returning %s",
                -et_values[1], strerror(r), MblError_to_str(MblError::DBA_SdBusCallFailure));
            status.set(MblError::DBA_SdBusCallFailure);
        }
        // Flush close and unref the connection - 
        // Executes sd_bus_unref(), but first executes sd_bus_flush(3) as well as sd_bus_close(3)
        // Always returns NULL 
        connection_handle_ = sd_bus_flush_close_unref(connection_handle_);
    }
    else {
        tr_error("bus_deinit called when connection_handle_ is NULL!");
    }
    
    return status.get();
}

MblError DBusAdapterImpl::event_loop_init()
{    
    tr_debug("Enter");
    int r = -1;
    
    // Create the sd-event loop object (thread loop)
    r = sd_event_default(&event_loop_handle_);
    if (r < 0){
        tr_error("sd_event_default failed with error r=%d (%s) - returning %s",
            r, strerror(r), MblError_to_str(MblError::DBA_SdEventCallFailure));
        return MblError::DBA_SdEventCallFailure;
    }
    tr_info("Acquired an event loop object! (event_loop_handle_=%p)", event_loop_handle_);

    // Attach input side of mailbox into the event loop as an input event source
    // This other side of the mailbox get output from other threads who whish to communicate with 
    // CCRB thread
    // Event source will be destroyed with the event loop ("floating")
    // Wait for event flag EPOLLIN (The associated file is available for read(2) operations)
    // The callback to invoke when the event is fired is incoming_mailbox_message_callback()
    r = sd_event_add_io(
        event_loop_handle_, 
        nullptr,    
        mailbox_in_.get_pipefd_read(), 
        EPOLLIN, 
        DBusAdapterImpl::incoming_mailbox_message_callback,
        this);
    if (r < 0){
        tr_error("sd_event_add_io failed with error r=%d (%s) - returning %s",
            r, strerror(r), MblError_to_str(MblError::DBA_SdEventCallFailure));
        return MblError::DBA_SdEventCallFailure;
    }
    tr_info("Added floating IO (input) event source to attach output from mailbox)", 
        event_loop_handle_);

    return MblError::None;
}

MblError DBusAdapterImpl::event_loop_deinit()
{    
    tr_debug("Enter");
    
    if (event_loop_handle_){       
        sd_event_unref(event_loop_handle_);
        event_loop_handle_ = nullptr;
    }
    else {
        tr_warn("event_loop_deinit called when event_loop_handle_ is NULL!");
    }

    return MblError::None;
}


MblError DBusAdapterImpl::event_loop_request_stop(MblError stop_status) 
{
    tr_debug("Enter");
    OneSetMblError status;

    // Only my CCRB thread is allowed to call this one, check no other thread is calling
    // Other threads (mbl-cloud-client main thread) should send a DBUS_ADAPTER_MSG_EXIT message via a
    // dedicated mailbox
    if (pthread_equal(pthread_self(), initializer_thread_id_) == 0){
         tr_error("Only CCRB thread should call event_loop_request_stop() - returning %s",
            MblError_to_str(MblError::DBA_ForbiddenCall));
        return MblError::DBA_ForbiddenCall;
    }

    // Send myself an exit request
    int r = sd_event_exit(event_loop_handle_, (int)stop_status);
    if (r < 0){
        tr_error("sd_event_exit failed with error r=%d (%s) - returning %s",
            r, strerror(r), MblError_to_str(MblError::DBA_SdEventExitRequestFailure));
            status.set(MblError::DBA_SdEventExitRequestFailure);
    }
    else {
        tr_info("sd_event_exit called with stop_status=%d", stop_status);
    }
    return status.get();
}

int DBusAdapterImpl::name_changed_match_callback_impl(
    sd_bus_message *m, sd_bus_error *ret_error)
{
    tr_debug("Enter");
    UNUSED(ret_error);
    UNUSED(m);

    // TODO - IMPLEMENT one or all of the 3:
    // org.freedesktop.DBus.NameOwnerChanged
    // org.freedesktop.DBus.NameLost
    // org.freedesktop.DBus.NameAcquired
    //assert(0);

    return (-EINVAL); // FIXME
}

int DBusAdapterImpl::name_changed_match_callback(
    sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    // TODO - IMPLEMENT
    tr_debug("Enter");
    DBusAdapterImpl *adapter_impl = static_cast<DBusAdapterImpl*>(userdata);
    return adapter_impl->name_changed_match_callback_impl(m, ret_error);   
}

int DBusAdapterImpl::incoming_mailbox_message_callback_impl(
    sd_event_source *s, 
    int fd,
 	uint32_t revents)
{
    tr_debug("Enter");
    UNUSED(s);
    MblError status = MblError::Unknown;
    MailboxMsg msg;

    // Validate that revents contains epoll read event flag
    if ((revents & EPOLLIN) == 0){
        // TODO : not sure if this error is possible - if it is - 
        // we need to restart thread/process or target (??)

        tr_err("(revents & EPOLLIN == 0), returning -1 to  disable event source");
        return (-1);
    }

    // Another validation - given fd is the one belongs to the mailbox (input side)
    if (fd != mailbox_in_.get_pipefd_read()){
        // TODO : handle on upper layer - need to notify somehow
        tr_err("fd does not belong to incoming mailbox_in_,"
            "returning -1 to disable event source");
        return (-1);
    }

    // Read the incoming message pointer. BLock for up to 1sec (we should block at all in practice)
    status = mailbox_in_.receive_msg(
        msg, 
        Mailbox::MAILBOX_MAX_POLLING_TIME_MILLISECONDS);
    if (status != MblError::None) {
         tr_err("mailbox_in_.receive_msg failed with status=%s, disable event source!",
            MblError_to_str(status));
        return (-1);
    }

    // Process message
    switch (msg.type_)
    {
        case mbl::MailboxMsg::MsgType::EXIT:
            // EXIT message
            
            //validate length (sanity check)
            if (msg.payload_len_ != sizeof(mbl::MailboxMsg::Msg_exit_)){
                tr_err("Unexpected EXIT message length %z (expected %d), returning error=%s",
                    msg.payload_len_, sizeof(mbl::MailboxMsg::Msg_exit_),
                    MblError_to_str(MblError::DBA_MailBoxInvalidMsg));
                return MblError::DBA_MailBoxInvalidMsg;
            }

            // External thread request to stop event loop
            tr_info("receive message EXIT : sending stop request to event loop with stop status=%s",
                 MblError_to_str(msg.payload_.exit.stop_status));
            status = event_loop_request_stop(msg.payload_.exit.stop_status);
            if (status != MblError::None) {
                tr_err("event_loop_request_stop failed with status=%s, disable event source!",
                MblError_to_str(status));
                return (-1);
            }
            break;

        case mbl::MailboxMsg::MsgType::RAW_DATA:
            // (used for testing) - ignore
            break;
    
        default:
            // This should never happen
            tr_err("Unexpected MsgType.. Ignoring..")
            break;
    }

    return 0; //success
}

int  DBusAdapterImpl::incoming_mailbox_message_callback(
    sd_event_source *s, 
    int fd,
 	uint32_t revents,
    void *userdata)
{
    tr_debug("Enter");
    DBusAdapterImpl *adapter_impl = static_cast<DBusAdapterImpl*>(userdata);
    return adapter_impl->incoming_mailbox_message_callback_impl(s, fd, revents);
}

int DBusAdapterImpl::incoming_bus_message_callback(
    sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    tr_debug("Enter");
    assert(userdata);

    //TODO - For all failues here, might need to send an error reply ONLY if the message is of 
    // kind method_call (can check that) check what is done in other implementations
    // see https://www.freedesktop.org/software/systemd/man/sd_bus_message_get_type.html#

    if (sd_bus_message_is_empty(m)){
        tr_err("Received an empty message!");
        return (-EINVAL);
    }
    
    // Expect message with our known name, directly sent to us (unicast)
    if (0 != strncmp(
        sd_bus_message_get_destination(m),
        DBUS_CLOUD_SERVICE_NAME, 
        strlen(DBUS_CLOUD_SERVICE_NAME)))
    {
        tr_err("Received message to wrong destination (%s)!", sd_bus_message_get_destination(m));
        return (-EINVAL);        
    }
    
    // Expect message to a single object patch DBUS_CLOUD_CONNECT_OBJECT_PATH
    if (0 != strncmp(
        sd_bus_message_get_path(m),
        DBUS_CLOUD_CONNECT_OBJECT_PATH, 
        strlen(DBUS_CLOUD_CONNECT_OBJECT_PATH)))
    {
        tr_err("Unexisting object path (%s)!", sd_bus_message_get_path(m));
        return (-EINVAL);        
    }

    // Expect message to a single interface DBUS_CLOUD_CONNECT_INTERFACE_NAME
    if (0 != strncmp(
        sd_bus_message_get_interface(m),
        DBUS_CLOUD_CONNECT_INTERFACE_NAME, 
        strlen(DBUS_CLOUD_CONNECT_INTERFACE_NAME)))
    {
        tr_err("Unexisting interface (%s)!", sd_bus_message_get_interface(m));
        return (-EINVAL);        
    }
    
    /*
    At this stage we are sure to handle all messages to known service DBUS_CLOUD_SERVICE_NAME,
    Object DBUS_CLOUD_CONNECT_OBJECT_PATH and interface DBUS_CLOUD_CONNECT_INTERFACE_NAME
    Messages can be of type Signal / Error / Method Call.
    */
    uint8_t type = 0;
    int r = sd_bus_message_get_type(m, &type);
    if (r < 0){
       tr_error("sd_bus_message_get_type failed with error r=%d (%s)",r, strerror(r));
       return r;
    } 
    if (false == is_valid_message_type(type)){
        tr_error("Invalid message type %d returned by sd_bus_message_get_type!", type);
        return (-EINVAL);
    }

    tr_info("Received message of type %s from sender %s",
        message_type_to_str(type), sd_bus_message_get_sender(m));

    DBusAdapterImpl *impl = static_cast<DBusAdapterImpl*>(userdata);
    if (sd_bus_message_is_method_call(m, 0, DBUS_CC_REGISTER_RESOURCES_METHOD_NAME)){
        r = impl->process_message_RegisterResources(m, ret_error);
        if(r < 0){
            tr_error("process_message_RegisterResources failed!");
            return (-EINVAL);
        }
    }
    else if (sd_bus_message_is_method_call(m, 0, DBUS_CC_DEREGISTER_RESOURCES_METHOD_NAME)) {
        r = impl->process_message_DeregisterResources(m, ret_error);
        if(r < 0){
            tr_error("process_message_DeregisterResources failed!");
            return (-EINVAL);
        }
    }
    else {
        //TODO - probably need to reply with error reply to sender?
        tr_err("Received a message with unknown member=%s!", sd_bus_message_get_member(m));
        assert(0);
        return (-EINVAL);
    }

    return 0;
}


int DBusAdapterImpl::method_reply_on_message(
    sd_bus_message *message_to_reply_on,
    sd_bus_error *ret_error,
    const char *types_format, ... /*values*/)
{
    UNUSED(ret_error);
    UNUSED(types_format);
    
    tr_debug("Enter");
    assert(message_to_reply_on);
    assert(types_format);
    
    const char* method_name = sd_bus_message_get_member(message_to_reply_on);    
    const char* sender_name = sd_bus_message_get_sender(message_to_reply_on);
    UNUSED(sender_name);
    sd_bus_message *reply = nullptr;
    sd_objects_cleaner<sd_bus_message> ref_cleaner (&reply, sd_bus_message_unrefp);
 
    int r = sd_bus_message_new_method_return(message_to_reply_on, &reply);
    if (r < 0){
        tr_error("sd_bus_message_new_method_return failed(err=%d) in reply to %s", 
                r, sender_name);  
        return (-EINVAL);
    }

    va_list arg_list;
    va_start(arg_list, types_format);
    if(0 == strncmp(method_name, 
                DBUS_CC_REGISTER_RESOURCES_METHOD_NAME, 
                strlen(DBUS_CC_REGISTER_RESOURCES_METHOD_NAME))){
        // we expect types_format = "us" for RegisterResources
        uint32_t cc_status = va_arg( arg_list, uint32_t);
        const char* access_token = va_arg( arg_list, const char*);
        r = sd_bus_message_append(reply, types_format, cc_status, access_token);
    } else if(0 == strncmp(method_name, 
                       DBUS_CC_DEREGISTER_RESOURCES_METHOD_NAME, 
                       strlen(DBUS_CC_DEREGISTER_RESOURCES_METHOD_NAME))){
        // we expect types_format = "u" for DeregisterResources
        uint32_t cc_status = va_arg( arg_list, uint32_t);
        r = sd_bus_message_append(reply, types_format, cc_status);
    } else {
        tr_error("Unexpected method name (%s) in reply to %s", 
                method_name, sender_name); 
        assert(0);
    }
    va_end(arg_list);

    if (r < 0){
        tr_error("sd_bus_message_append with types_format=%s failed(err=%d) in reply to %s", 
                types_format, r, sender_name);  
        return (-EINVAL);
    }
    
    r = sd_bus_send(connection_handle_, reply, nullptr);
    if (r < 0){
        tr_error("sd_bus_send failed(err=%d) in reply to %s", r, sender_name);  
        return (-EINVAL);
    }

    tr_debug("Reply on %s successfully sent to %s", method_name, sender_name);  

    return 0;
}

int DBusAdapterImpl::reply_error_on_message(
    sd_bus_message *message_to_reply_on,
    sd_bus_error *ret_error,
    const CloudConnectStatus error)
{
    UNUSED(ret_error);

    tr_debug("Enter");
    assert(message_to_reply_on);
    const char* method_name = sd_bus_message_get_member(message_to_reply_on);    
    const char* sender_name = sd_bus_message_get_sender(message_to_reply_on);
    UNUSED(sender_name);
    UNUSED(method_name);

    int r = sd_bus_reply_method_errorf(
        message_to_reply_on, 
        CloudConnectStatus_error_to_DBus_format_string(error),
        "%s", CloudConnectStatus_to_readable_string(error));
    if (r < 0){
        tr_error("sd_bus_reply_method_errorf failed(err=%d) in reply error to %s",
                r, sender_name);
        return (-EINVAL);
    }

    tr_debug("Reply error on method %s successfully sent to %s", method_name, sender_name);  

    return 0;
}

int DBusAdapterImpl::process_message_RegisterResources(
    sd_bus_message *m, 
    sd_bus_error *ret_error)
{
    UNUSED(ret_error);
    tr_debug("Enter");
    assert(m);
        
    tr_info("Starting to process RegisterResources method call from sender %s",
        sd_bus_message_get_sender(m));

    if (sd_bus_message_get_expect_reply(m) == 0) {
        // reply to the message m is not expected.
        tr_err("Unexpected message type: no reply expected");
        return (-EINVAL);
    }

    if (sd_bus_message_has_signature(m, "s") == false){
        tr_err("Unexpected signature %s", sd_bus_message_get_signature(m, 1));
        return (-EINVAL);        
    }

    const char *json_file_data = nullptr;       
    int r = sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &json_file_data);
    if (r < 0){
        tr_error("sd_bus_message_read_basic failed with error r=%d (%s)",r, strerror(r));
        return r;
    }
    if ((nullptr == json_file_data) || (strlen(json_file_data) == 0)){
        tr_error("sd_bus_message_read_basic returned a nullptr or an empty string!");
        return (-EINVAL);
    }

    // TODO:
    // validate app registered expected interface on bus? (use sd-bus track)

    // call register_resources resource broker APi and handle output
    CloudConnectStatus out_cc_reg_status = ERR_FAILED;
    std::string out_access_token;
    MblError mbl_reg_err = ccrb_.register_resources(
        (uintptr_t)m, std::string(json_file_data), 
        out_cc_reg_status, out_access_token);            

    if(MblError::None != mbl_reg_err || is_CloudConnectStatus_error(out_cc_reg_status)){

        CloudConnectStatus reg_status_to_send = ERR_INTERNAL_ERROR;
        if(MblError::None != mbl_reg_err){
            // we have internal error in resource broker 
            tr_error("register_resources failed with MblError %s", MblError_to_str(mbl_reg_err));
        }else{
            // we have cloud connect related error in resource broker 
            tr_error("register_resources failed with cloud connect error %s", 
                CloudConnectStatus_stringify(out_cc_reg_status));
            reg_status_to_send = out_cc_reg_status;
        }

        r = reply_error_on_message(m, ret_error, reg_status_to_send);
        if(r < 0){
            tr_error("reply_error_on_message failed!");
            return (-EINVAL);
        }

        // altough register_resources failed, process_message_RegisterResources succeeded 
        return 0;
    }

    r = method_reply_on_message(m, ret_error, "us", out_cc_reg_status, out_access_token.c_str());
    if(r < 0){
        tr_error("method_reply_on_message failed!");
        return (-EINVAL);
    }

    // We have got success from register_resources. Register resources is an asynchronous 
    // process towards the cloud, so we need to store handle and increase refcount
    if (pending_messages_.insert(m).second == false){
        tr_error("pending_messages_.insert failed!");
        return (-EINVAL);
    }  

    sd_bus_message_ref(m);

/*
    // FOR SIGNAL TESTS PURPOSES ONLY
    handle_ccrb_async_process_status_update(
        (uintptr_t)m, 
        DBUS_CC_REGISTER_RESOURCES_STATUS_SIGNAL_NAME, 
        STATUS_SUCCESS);
*/
    return 0;
}


int DBusAdapterImpl::process_message_DeregisterResources(
    sd_bus_message *m, 
    sd_bus_error *ret_error) 
{
    UNUSED(ret_error);
    tr_debug("Enter");
    assert(m);
    tr_info("Starting to process DergisterResources method call from sender %s",
        sd_bus_message_get_sender(m));

    if (sd_bus_message_has_signature(m, "s") == false){
        tr_err("Unexpected signature %s", sd_bus_message_get_signature(m, 1));
        return (-EINVAL);        
    }
    
    const char *access_token = NULL;       
    int r = sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &access_token);
    if (r < 0){
        tr_error("sd_bus_message_read_basic failed with error r=%d (%s)",r, strerror(r));
        return r;
    }
    if ((nullptr == access_token) || (strlen(access_token) == 0)){
        tr_error("sd_bus_message_read_basic returned a nullptr or an empty string!");
        return (-EINVAL);
    }
     
    // call deregister_resources resource broker APi and handle output
    CloudConnectStatus out_cc_dereg_status = ERR_FAILED;
    MblError mbl_dereg_err = ccrb_.deregister_resources(
        (uintptr_t)m, std::string(access_token), 
        out_cc_dereg_status);            

    if(MblError::None != mbl_dereg_err || is_CloudConnectStatus_error(out_cc_dereg_status)){

        CloudConnectStatus dereg_status_to_send = ERR_INTERNAL_ERROR;
        if(MblError::None != mbl_dereg_err){
            // we have internal error in resource broker 
            tr_error("deregister_resources failed with MblError %s", MblError_to_str(mbl_dereg_err));
        }else{
            // we have cloud connect related error in resource broker 
            tr_error("deregister_resources failed with cloud connect error %s", 
                CloudConnectStatus_stringify(out_cc_dereg_status));
            dereg_status_to_send = out_cc_dereg_status;
        }

        r = reply_error_on_message(m, ret_error, dereg_status_to_send);
        if(r < 0){
            tr_error("reply_error_on_message failed!");
            return (-EINVAL);
        }

        // altough deregister_resources failed, process_message_DeregisterResources succeeded 
        return 0;
    }

    r = method_reply_on_message(m, ret_error, "u", out_cc_dereg_status);
    if(r < 0){
        tr_error("method_reply_on_message failed!");
        return (-EINVAL);
    }

    // We have got success from deregister_resources. Deregister resources is an asynchronous 
    // process towards the cloud, so we need to store handle and increase refcount
    sd_bus_message_ref(m);
    if (pending_messages_.insert(m).second == false){
        tr_error("pending_messages_.insert failed!");
        return (-EINVAL);
    }  

    return 0;
}

MblError DBusAdapterImpl::init()
{
    tr_debug("Enter");    
    MblError status = MblError::Unknown;    
    
    if (state_.is_not_equal(DBusAdapterState::UNINITALIZED)){
        tr_error("Unexpected state (expected %s), returning error %s", 
            state_.to_string(), MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }

    // set callback into DBusService C module
    DBusService_init(DBusAdapterImpl::incoming_bus_message_callback);
   
    // Init incoming message mailbox
    status = mailbox_in_.init();
    if (status != MblError::None){
        //mailbox deinit itself
        tr_error("mailbox_in_.init() failed with error %s", MblError_to_str(status));
        return status;
    }

    status = event_loop_init();
    if (status != MblError::None){
        //event loop is not an object, need to deinit        
        tr_error("event_loop_init() failed with error %s", MblError_to_str(status));
        event_loop_deinit();
        return status;
    }

    status = bus_init();
    if (status != Error::None){
        //bus is not an object, need to deinit
        tr_error("bus_init() failed with error %s", MblError_to_str(status));
        bus_deinit();        
        return status;
    }
    
    initializer_thread_id_ = pthread_self();
    state_.set(DBusAdapterState::INITALIZED);
    tr_info("init finished with SUCCESS!");
    return Error::None;
}

MblError DBusAdapterImpl::deinit()
{
    tr_debug("Enter");
    OneSetMblError ret_status;
    MblError status;
        
    if (state_.is_not_equal(DBusAdapterState::INITALIZED)){
        tr_error("Unexpected state (expected %s), returning error %s", 
            state_.to_string(), MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }

    // Perform a "best effort" deinit - continue on failure and return first error code (if happen)    
    status = mailbox_in_.deinit();
    if (status != MblError::None){
        tr_error("mailbox_in_.deinit failed with error %s", MblError_to_str(status));
        ret_status.set(status);
        //continue        
    }   
    
    status = bus_deinit();
    if (status != MblError::None){
        tr_error("bus_deinit() failed with error %s", MblError_to_str(status));
        ret_status.set(status);
        //continue        
    }

    status = event_loop_deinit();
    if (status != MblError::None){
        tr_error("mailbox_in_.deinit failed with error %s", MblError_to_str(status));
        ret_status.set(status);
        //continue        
    }

    DBusService_deinit();

    // TODO - clean all references to messages, timers etc. to unref all
    //for (auto &handle : bus_request_handles_){
        //sd_bus_message_unref((sd_bus_message*)handle);
    //}

    state_.set(DBusAdapterState::UNINITALIZED);
    if (MblError::None == ret_status.get()){
        tr_info("Deinit finished with SUCCESS!");
    }
    return ret_status.get();
}


MblError DBusAdapterImpl::event_loop_run(MblError &stop_status) 
{
    tr_debug("Enter - Start running!");

    /*
    Thread enters the sd-event loop and blocks. See:
    https://www.freedesktop.org/software/systemd/man/sd_event_run.html# :
    sd_event_loop() invokes sd_event_run() in a loop, 
    thus implementing the actual event loop. The call returns as soon as exiting 
    was requested using sd_event_exit(3).
    sd_event_loop() returns the exit code specified when invoking sd_event_exit()
    */
    state_.set(DBusAdapterState::RUNNING);
    stop_status = (MblError)sd_event_loop(event_loop_handle_);
    state_.set(DBusAdapterState::INITALIZED);
    return MblError::None;
}

MblError DBusAdapterImpl::run(MblError &stop_status)
{
    tr_debug("Enter");
    MblError status = MblError::Unknown;

    if (state_.is_not_equal(DBusAdapterState::INITALIZED)){
        tr_error("Unexpected state (expected %s), returning error %s", 
            state_.to_string(), MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }
        
    status = event_loop_run(stop_status);    
    if (status != MblError::None){
        tr_error("event_loop_run() failed with error %s", MblError_to_str(status));
        return status;
    }
    state_.set(DBusAdapterState::INITALIZED);

    return MblError::None;
}

MblError DBusAdapterImpl::stop(MblError stop_status)
{
    tr_debug("Enter");
    MblError status = MblError::Unknown;

    if (state_.is_equal(DBusAdapterState::UNINITALIZED)){
        tr_error("Unexpected state (expected %s), returning error %s", 
            state_.to_string(), MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }

    if (pthread_equal(pthread_self(), initializer_thread_id_) != 0){
        // This section is for self exit request, use event_loop_request_stop
        // current thread id ==  initializer_thread_id_
        int r = event_loop_request_stop(stop_status);
        if (r < 0){
            tr_error("event_loop_request_stop failed with error r=%d (%s) - returning %s",
                r, strerror(r), MblError_to_str(MblError::DBA_SdEventCallFailure));
            status = MblError::DBA_SdEventCallFailure;
            //continue
        }
        else {
            tr_info("Sent self request to exit sd-event loop!");
        }
    }
    else {
        // This section is for external threads exit requests - send EXIT message to mailbox_in_
        // Thread shouldn't block here, but we still supply a maximum timeout of 
        // MSG_SEND_ASYNC_TIMEOUT_MILLISECONDS
        MailboxMsg msg;
        msg.type_ = mbl::MailboxMsg::MsgType::EXIT;
        msg.payload_len_ = sizeof(mbl::MailboxMsg::Msg_exit_);
        msg.payload_.exit.stop_status = stop_status;
        status = mailbox_in_.send_msg(msg, Mailbox::MAILBOX_MAX_POLLING_TIME_MILLISECONDS);
        if (status != MblError::None){
            tr_err("mailbox_in_.send_msg failed with error %s", MblError_to_str(status));
            status = MblError::DBA_SdEventCallFailure;
            //continue
        }
        else {
            tr_info("Sent request to stop CCRB thread inside sd-event loop!");
        }
    }

    return status;
}


MblError DBusAdapterImpl::handle_ccrb_async_process_status_update(
    const uintptr_t ipc_request_handle, 
    const char* signal_name,
    const CloudConnectStatus status)
{
    tr_debug("Enter");   
    assert(ipc_request_handle);
    assert(signal_name);

    if (state_.is_not_equal(DBusAdapterState::RUNNING)){
        tr_error("Unexpected state (expected %s), returning error %s", 
            state_.to_string(), MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }

    sd_bus_message *message_to_signal_on = reinterpret_cast<sd_bus_message *>(ipc_request_handle);

    // try find handle in pending_messages
    auto it = pending_messages_.find(message_to_signal_on);
    if(pending_messages_.end() == it){
        // handle provided was not previously stored!  
        tr_error("provided handle (expected 0x%x) not found in pending messages, returning error %s", 
            ipc_request_handle, MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }

    // remove message from pending messages
    pending_messages_.erase (it);
    
    // message_to_signal_on's refcount should be reduces 
    sd_objects_cleaner<sd_bus_message> mess_cleaner (&message_to_signal_on, sd_bus_message_unrefp);

    sd_bus_message *signal = NULL;
    sd_objects_cleaner<sd_bus_message> sig_cleaner (&signal, sd_bus_message_unrefp);

    int r = sd_bus_message_new_signal(
        connection_handle_, &signal, 
        DBUS_CLOUD_CONNECT_OBJECT_PATH, 
        DBUS_CLOUD_CONNECT_INTERFACE_NAME, 
        signal_name);
    if(r < 0){
        tr_error("sd_bus_message_new_signal failed!");
        return MblError::DBA_SdBusCallFailure;
    }

    // set destination of signal message
    const char *signal_dest = sd_bus_message_get_sender(message_to_signal_on);
    r = sd_bus_message_set_destination(signal, signal_dest);
    if (r < 0){
        tr_error("sd_bus_message_set_destination failed(err=%d)", r);  
        return MblError::DBA_SdBusCallFailure;
    }

    // append status
    r = sd_bus_message_append(signal, "u", status);
    if (r < 0){
        tr_error("sd_bus_message_append %s failed(err=%d)", "u", r);  
        return MblError::DBA_SdBusCallFailure;
    }

    r = sd_bus_send(connection_handle_, signal, NULL);
    if (r < 0){
        tr_error("signal_on_async_process_finished failed!");
        return MblError::DBA_SdBusCallFailure;
    }
    
    tr_debug("Signal %s was successfully emitted to %s", signal_name, signal_dest);  

    return MblError::None;
}

bool DBusAdapterImpl::is_valid_message_type(uint8_t message_type)
{
    switch (message_type) {
        case SD_BUS_MESSAGE_METHOD_CALL:    return true;
        case SD_BUS_MESSAGE_METHOD_RETURN:  return true;
        case SD_BUS_MESSAGE_METHOD_ERROR:   return true;
        case SD_BUS_MESSAGE_SIGNAL:         return true;
        default: return false;
    }
}

//https://www.freedesktop.org/software/systemd/man/sd_bus_message_get_type.html#
const char *DBusAdapterImpl::message_type_to_str(uint8_t message_type)
{
    switch (message_type) {
        case SD_BUS_MESSAGE_METHOD_CALL:    return stringify(SD_BUS_MESSAGE_METHOD_CALL);
        case SD_BUS_MESSAGE_METHOD_RETURN:  return stringify(SD_BUS_MESSAGE_METHOD_RETURN);
        case SD_BUS_MESSAGE_METHOD_ERROR:   return stringify(SD_BUS_MESSAGE_METHOD_ERROR);
        case SD_BUS_MESSAGE_SIGNAL:         return stringify(SD_BUS_MESSAGE_SIGNAL);
        default: return "";
    }
}

const char *DBusAdapterImpl::State::to_string()
{
    switch (current_)
    {
        case eState::UNINITALIZED: return "UNINITALIZED";
        case eState::INITALIZED: return "INITALIZED";
        case eState::RUNNING: return "RUNNING";
        default: 
            assert(0); //should never happen!
            return ""; 
    }
}

void DBusAdapterImpl::State::set(eState new_state) 
{
    if (current_ != new_state){
        current_ = new_state;
        tr_info("New adapter state %s", to_string());
    }
}

DBusAdapterImpl::DBusAdapterState DBusAdapterImpl::State::get() 
{   
    return current_;
}

bool DBusAdapterImpl::State::is_equal(eState state)
{
    return (current_ == state);
}

bool DBusAdapterImpl::State::is_not_equal(eState state)
{
    return (current_ != state);
}

} // namespace mbl {