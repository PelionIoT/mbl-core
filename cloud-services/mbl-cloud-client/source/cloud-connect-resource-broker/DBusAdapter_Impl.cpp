/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "CloudConnectTrace.h"
#include "DBusAdapter.h"
#include "Mailbox.h"
#include "MailboxMsg.h"
#include "DBusService.h"
#include "DBusAdapter_Impl.h"
#include "ResourceBroker.h"
#include "CloudConnectTypes.h"

#include <cassert>
#include <vector>
#include <string>

#define TRACE_GROUP "ccrb-dbus"

namespace mbl {

DBusAdapterImpl::DBusAdapterImpl(ResourceBroker &ccrb) :
    ccrb_(ccrb),        
    connection_handle_(nullptr),
    unique_name_(nullptr),
    service_name_(nullptr),
    event_loop_handle_(nullptr),
    mailbox_in_("incoming messages mailbox"),    
    initializer_thread_id_(0)
{
    TR_DEBUG("Enter");
    state_.set(DBusAdapterState::UNINITALIZED);
}


MblError DBusAdapterImpl::bus_init()
{
    TR_DEBUG("Enter");    

    // Enforce initialization of event loop before bus
    if (nullptr == event_loop_handle_){
        TR_ERR("event_loop_handle_ not initialized! returning %s",
            MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }   

    // Open a connection to the bus. DBUS_SESSION_BUS_ADDRESS should be define as part of the 
    // process environment
    int r = sd_bus_open_user(&connection_handle_);    
    if (r < 0){
         TR_ERR("sd_bus_open_user failed with error r=%d (%s) - returning %s",
            r, strerror(r), MblError_to_str(MblError::DBA_SdBusCallFailure));
        return MblError::DBA_SdBusCallFailure;
    }
    assert(connection_handle_);
    TR_INFO("D-Bus Connection object created (connection_handle_=%p)", connection_handle_);

    // Attach bus connection object to event loop
    r = sd_bus_attach_event(connection_handle_, event_loop_handle_, SD_EVENT_PRIORITY_NORMAL);
    if (r < 0){
        TR_ERR("sd_bus_attach_event failed with error r=%d (%s) - returning %s",
            r, strerror(r), MblError_to_str(MblError::DBA_SdBusCallFailure));
        return MblError::DBA_SdBusCallFailure;
    }
    TR_INFO("Connection object attached to event object");

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
        TR_ERR("sd_bus_add_object_vtable failed with error r=%d (%s) - returning %s",
            r, strerror(r), MblError_to_str(MblError::DBA_SdBusCallFailure));
        return MblError::DBA_SdBusCallFailure;
    }
    TR_INFO("Added new interface %s using service_vtable to object %s",
        DBUS_CLOUD_CONNECT_INTERFACE_NAME,
        DBUS_CLOUD_CONNECT_OBJECT_PATH);

    // Get my unique name on the bus    
    r = sd_bus_get_unique_name(connection_handle_, &unique_name_);
    if (r < 0) {
         TR_ERR("sd_bus_get_unique_name failed with error r=%d (%s) - returning %s",
            r, strerror(r), MblError_to_str(MblError::DBA_SdBusCallFailure));
        return MblError::DBA_SdBusCallFailure;
    }
    assert(unique_name_);
    TR_INFO("unique_name_=%s", unique_name_);
    
    // Request a well-known service name DBUS_CLOUD_SERVICE_NAME so client Apps can find us
    // We do not expect anyone else to already own that name
    r = sd_bus_request_name(connection_handle_, DBUS_CLOUD_SERVICE_NAME, 0);
    if (r < 0) {
        TR_ERR("sd_bus_request_name failed with error r=%d (%s) - returning %s",
            r, strerror(r), MblError_to_str(MblError::DBA_SdBusRequestNameFailed));
        return MblError::DBA_SdBusRequestNameFailed;
    }
    service_name_ = DBUS_CLOUD_SERVICE_NAME;
    TR_INFO("Aquired D-Bus known name service_name_=%s", DBUS_CLOUD_SERVICE_NAME);
    
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
        TR_ERR("sd_bus_add_match failed with error r=%d (%s) - returning %s",
            r, strerror(r), MblError_to_str(MblError::DBA_SdBusRequestAddMatchFailed));
        return MblError::DBA_SdBusRequestAddMatchFailed;
    }
    TR_INFO("Added D-Bus broker signal match - NameOwnerChanged");

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
        TR_ERR("sd_bus_add_match failed with error r=%d (%s) - returning %s",
            r, strerror(r), MblError_to_str(MblError::DBA_SdBusRequestAddMatchFailed));
        return MblError::DBA_SdBusRequestAddMatchFailed;
    }
    TR_INFO("Added D-Bus broker signal match - NameLost");

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
        TR_ERR("sd_bus_add_match failed with error r=%d (%s) - returning %s",
            r, strerror(r), MblError_to_str(MblError::DBA_SdBusRequestAddMatchFailed));
        return MblError::DBA_SdBusRequestAddMatchFailed;
    }
    TR_INFO("Added D-Bus broker signal match - NameAcquired");
   
    return MblError::None;
}

MblError DBusAdapterImpl::bus_deinit()
{
    TR_DEBUG("Enter");
    
    // sd_bus_flush_close_unref return always NULL
    connection_handle_ = sd_bus_flush_close_unref(connection_handle_);
    service_name_ = nullptr;
    unique_name_ = nullptr;
 
    return MblError::None;
}

MblError DBusAdapterImpl::event_loop_init()
{    
    TR_DEBUG("Enter");

    // Create the sd-event loop object (thread loop)
    int r = sd_event_default(&event_loop_handle_);
    if (r < 0){
        TR_ERR("sd_event_default failed with error r=%d (%s) - returning %s",
            r, strerror(r), MblError_to_str(MblError::DBA_SdEventCallFailure));
        return MblError::DBA_SdEventCallFailure;
    }
    assert(event_loop_handle_);
    TR_INFO("Acquired an event loop object! (event_loop_handle_=%p)", event_loop_handle_);

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
        TR_ERR("sd_event_add_io failed with error r=%d (%s) - returning %s",
            r, strerror(r), MblError_to_str(MblError::DBA_SdEventCallFailure));
        return MblError::DBA_SdEventCallFailure;
    }
    TR_INFO("Added floating IO (input) event source to attach output from mailbox)");

    return MblError::None;
}

MblError DBusAdapterImpl::event_loop_deinit()
{    
    TR_DEBUG("Enter");
    
    if (event_loop_handle_){       
        sd_event_unref(event_loop_handle_);
        event_loop_handle_ = nullptr;
    }
    else {
        TR_WARN("event_loop_deinit called when event_loop_handle_ is NULL!");
    }

    return MblError::None;
}


MblError DBusAdapterImpl::event_loop_request_stop(MblError stop_status) 
{
    TR_DEBUG("Enter");
    OneSetMblError status;

    // Only my CCRB thread is allowed to call this one, check no other thread is calling
    // Other threads (mbl-cloud-client main thread) should send a DBUS_ADAPTER_MSG_EXIT message 
    // via a dedicated mailbox
    if (pthread_equal(pthread_self(), initializer_thread_id_) == 0){
         TR_ERR("Only CCRB thread should call event_loop_request_stop() - returning %s",
            MblError_to_str(MblError::DBA_ForbiddenCall));
        return MblError::DBA_ForbiddenCall;
    }

    // Send myself an exit request
    int r = sd_event_exit(event_loop_handle_, (int)stop_status);
    if (r < 0){
        status.set(MblError::DBA_SdEventExitRequestFailure);
        TR_ERR("sd_event_exit failed with error r=%d (%s) - returning %s",
            r, strerror(r), status.get_status_str());
            
    }
    else {
        TR_INFO("sd_event_exit called with stop_status=%d", (int)stop_status);
    }
    return status.get();
}

int DBusAdapterImpl::name_changed_match_callback_impl(
    sd_bus_message *m, sd_bus_error *ret_error)
{
    assert(m);
    assert(ret_error);
    TR_DEBUG("Enter");
    UNUSED(ret_error);
    UNUSED(m);

    // TODO - IMPLEMENT one or all of the 3:
    // org.freedesktop.DBus.NameOwnerChanged
    // org.freedesktop.DBus.NameLost
    // org.freedesktop.DBus.NameAcquired

    //return (-EINVAL); // FIXME
    return 0; //temporary
}

int DBusAdapterImpl::name_changed_match_callback(
    sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    assert(m);
    assert(ret_error);
    assert(userdata);
    // TODO - IMPLEMENT
    TR_DEBUG("Enter");
    DBusAdapterImpl *adapter_impl = static_cast<DBusAdapterImpl*>(userdata);
    return adapter_impl->name_changed_match_callback_impl(m, ret_error);   
}

int DBusAdapterImpl::incoming_mailbox_message_callback_impl(
    sd_event_source *s, 
    int fd,
 	uint32_t revents)
{
    assert(s);
    TR_DEBUG("Enter");
    UNUSED(s);
    MblError status = MblError::Unknown;

    // Validate that revents contains epoll read event flag
    if ((revents & EPOLLIN) == 0){
        // TODO : not sure if this error is possible - if it is - 
        // we need to restart thread/process or target (??)

        TR_ERR("(revents & EPOLLIN == 0), returning -1 to  disable event source");
        return (-1);
    }

    // Another validation - given fd is the one belongs to the mailbox (input side)
    if (fd != mailbox_in_.get_pipefd_read()){
        // TODO : handle on upper layer - need to notify somehow
        TR_ERR("fd does not belong to incoming mailbox_in_,"
            "returning -1 to disable event source");
        return (-1);
    }

    // Read the incoming message pointer. BLock for up to 1sec (we should block at all in practice)
    auto ret_pair = mailbox_in_.receive_msg();
    if (ret_pair.first != MblError::None) {
         TR_ERR("mailbox_in_.receive_msg failed with status=%s, disable event source!",
            MblError_to_str(status));
        return (-1);
    }

    // Process message
    MailboxMsg &msg = ret_pair.second;
    switch (msg.get_type())
    {
        case mbl::MailboxMsg::MsgType::EXIT:
        {
            // EXIT message
            
            //validate length (sanity check)
            if (msg.get_payload_len() != sizeof(mbl::MailboxMsg::MsgPayload::MsgExit)){
                TR_ERR("Unexpected EXIT message length %zu (expected %zu), returning error=%s",
                    msg.get_payload_len(),
                    sizeof(mbl::MailboxMsg::MsgType::EXIT),
                    MblError_to_str(MblError::DBA_MailBoxInvalidMsg));
                return MblError::DBA_MailBoxInvalidMsg;
            }

            // External thread request to stop event loop
            auto &payload = msg.get_payload();
            TR_INFO("receive message EXIT : sending stop request to event loop with stop status=%s",
                 MblError_to_str(payload.exit_.stop_status));
            status = event_loop_request_stop(payload.exit_.stop_status);
            if (status != MblError::None) {
                TR_ERR("event_loop_request_stop failed with status=%s, disable event source!",
                MblError_to_str(status));
                return (-1);
            }
            break;
        }
        case mbl::MailboxMsg::MsgType::RAW_DATA:
            // (used for testing) - ignore
            break;
    
        default:
            // This should never happen
            TR_ERR("Unexpected MsgType.. Ignoring..");
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
    assert(s);
    assert(userdata);
    TR_DEBUG("Enter");
    DBusAdapterImpl *adapter_impl = static_cast<DBusAdapterImpl*>(userdata);
    return adapter_impl->incoming_mailbox_message_callback_impl(s, fd, revents);
}

int DBusAdapterImpl::incoming_bus_message_callback(
    sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    TR_DEBUG("Enter");
    assert(userdata);
    assert(m);
    assert(ret_error);
    MblError status;

    //TODO - For all failues here, might need to send an error reply ONLY if the message is of 
    // kind method_call (can check that) check what is done in other implementations
    // see https://www.freedesktop.org/software/systemd/man/sd_bus_message_get_type.html#

    if (sd_bus_message_is_empty(m)){
        TR_ERR("Received an empty message!");
        return (-EINVAL);
    }
    
    // Expect message with our known name, directly sent to us (unicast)
    if (0 != strncmp(
        sd_bus_message_get_destination(m),
        DBUS_CLOUD_SERVICE_NAME, 
        strlen(DBUS_CLOUD_SERVICE_NAME)))
    {
        TR_ERR("Received message to wrong destination (%s)!", sd_bus_message_get_destination(m));
        return (-EINVAL);        
    }
    
    // Expect message to a single object patch DBUS_CLOUD_CONNECT_OBJECT_PATH
    if (0 != strncmp(
        sd_bus_message_get_path(m),
        DBUS_CLOUD_CONNECT_OBJECT_PATH, 
        strlen(DBUS_CLOUD_CONNECT_OBJECT_PATH)))
    {
        TR_ERR("Unexisting object path (%s)!", sd_bus_message_get_path(m));
        return (-EINVAL);        
    }

    // Expect message to a single interface DBUS_CLOUD_CONNECT_INTERFACE_NAME
    if (0 != strncmp(
        sd_bus_message_get_interface(m),
        DBUS_CLOUD_CONNECT_INTERFACE_NAME, 
        strlen(DBUS_CLOUD_CONNECT_INTERFACE_NAME)))
    {
        TR_ERR("Unexisting interface (%s)!", sd_bus_message_get_interface(m));
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
       TR_ERR("sd_bus_message_get_type failed with error r=%d (%s)",r, strerror(r));
       return r;
    } 
    if (false == is_valid_message_type(type)){
        TR_ERR("Invalid message type %" PRIu8 " returned by sd_bus_message_get_type!", type);
        return (-EINVAL);
    }

    TR_INFO("Received message of type %s from sender %s",
        message_type_to_str(type), sd_bus_message_get_sender(m));

    DBusAdapterImpl *impl = static_cast<DBusAdapterImpl*>(userdata);
    if (sd_bus_message_is_method_call(m, 0, "RegisterResources")){
        r = impl->process_message_RegisterResources(m, ret_error);
        UNUSED(status); //FIXME
        //TODO - handle return value and continue implementation
        assert(0);
    }
    else if (sd_bus_message_is_method_call(m, 0, "DeregisterResources")) {
        r = impl->process_message_DeregisterResources(m, ret_error);
        UNUSED(status); 
        //TODO - handle return value and continue implementation
        assert(0);
    }
    else {
        //TODO - probably need to reply with error reply to sender?
        TR_ERR("Received a message with unknown member=%s!", sd_bus_message_get_member(m));
        assert(0);
        return (-EINVAL);
    }

    return 0;
}

// TODO - this function is incomplete!!
int DBusAdapterImpl::process_message_RegisterResources(
    sd_bus_message *m, 
    sd_bus_error *ret_error)
{
    assert(ret_error);
    assert(m);
    UNUSED(ret_error);
    TR_DEBUG("Enter");
    const char *json_file_data = nullptr;       
        
    TR_INFO("Starting to process RegisterResources method call from sender %s",
        sd_bus_message_get_sender(m));

    if (sd_bus_message_has_signature(m, "s") == false){
        TR_ERR("Unexpected signature %s", sd_bus_message_get_signature(m, 1));
        return (-EINVAL);        
    }
    int r = sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &json_file_data);
    if (r < 0){
        TR_ERR("sd_bus_message_read_basic failed with error r=%d (%s)",r, strerror(r));
        return r;
    }
    assert(json_file_data);
    if ((nullptr == json_file_data) || (strlen(json_file_data) == 0)){
        TR_ERR("sd_bus_message_read_basic returned a nullptr or an empty string!");
        return (-EINVAL);
    }
     
    // Register resources is an asynchronous process towards the cloud -> store handle and
    // increase refcount
    sd_bus_message_ref(m);
    if (pending_messages_.insert(m).second == false){
        TR_ERR("pending_messages_.insert failed!");
        return (-EINVAL);
    }  

    // TODO:
    // validate app registered expected interface on bus? (use sd-bus track)
        
    //TODO - call CCRB    
    //ccrb_.register_resources();            

    //TODO - handle reply    
    assert(0);
    return 0;
}

int DBusAdapterImpl::process_message_DeregisterResources(
    sd_bus_message *m, 
    sd_bus_error *ret_error) 
{
    assert(m);
    assert(ret_error);
    UNUSED(ret_error);
    TR_DEBUG("Enter");
    const char *access_token = NULL;       
        
    TR_INFO("Starting to process DergisterResources method call from sender %s",
        sd_bus_message_get_sender(m));

    if (sd_bus_message_has_signature(m, "s") == false){
        TR_ERR("Unexpected signature %s", sd_bus_message_get_signature(m, 1));
        return (-EINVAL);        
    }
    int r = sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &access_token);
    if (r < 0){
        TR_ERR("sd_bus_message_read_basic failed with error r=%d (%s)",r, strerror(r));
        return r;
    }
    assert(access_token);
    if ((nullptr == access_token) || (strlen(access_token) == 0)){
        TR_ERR("sd_bus_message_read_basic returned a nullptr or an empty string!");
        return (-EINVAL);
    }
     
    // Deregister resources is an asynchronous process towards the cloud -> store handle and
    // increase refcount
    sd_bus_message_ref(m);
    if (pending_messages_.insert(m).second == false){
        TR_ERR("pending_messages_.insert failed!");
        return (-EINVAL);
    }  
        
    //TODO call CCRB    
    //ccrb_.deregister_resources();            

    //TODO - handle reply    
    assert(0);
    return 0;
}

MblError DBusAdapterImpl::init()
{
    TR_DEBUG("Enter");    
    MblError status = MblError::Unknown;    
    
    if (state_.is_not_equal(DBusAdapterState::UNINITALIZED)){
        TR_ERR("Unexpected state (expected %s), returning error %s", 
            state_.to_string(), MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }

    // set callback into DBusService C module
    DBusService_init(DBusAdapterImpl::incoming_bus_message_callback);
   
    // Init incoming message mailbox
    status = mailbox_in_.init();
    if (status != MblError::None){
        //mailbox deinit itself
        TR_ERR("mailbox_in_.init() failed with error %s", MblError_to_str(status));
        return status;
    }

    status = event_loop_init();
    if (status != MblError::None){
        //event loop is not an object, need to deinit        
        TR_ERR("event_loop_init() failed with error %s", MblError_to_str(status));
        event_loop_deinit();
        return status;
    }

    status = bus_init();
    if (status != Error::None){
        //bus is not an object, need to deinit
        TR_ERR("bus_init() failed with error %s", MblError_to_str(status));
        bus_deinit();        
        return status;
    }
    
    status = event_manager_.init();
    if (status != MblError::None){
        TR_ERR("event_manager_.init() failed with error %s", MblError_to_str(status));
        event_manager_.deinit();       
        return status;
    }

    initializer_thread_id_ = pthread_self();
    state_.set(DBusAdapterState::INITALIZED);
    TR_INFO("init finished with SUCCESS!");
    return Error::None;
}

MblError DBusAdapterImpl::deinit()
{
    TR_DEBUG("Enter");
    OneSetMblError one_set_status;
    MblError status;
        
    if (state_.is_not_equal(DBusAdapterState::INITALIZED)){
        TR_ERR("Unexpected state (expected %s), returning error %s", 
            state_.to_string(), MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }

    // Perform a "best effort" deinit - continue on failure and return first error code (if happen)    
    status = mailbox_in_.deinit();
    if (status != MblError::None){
        one_set_status.set(status);
        TR_ERR("mailbox_in_.deinit failed with error %s", MblError_to_str(status));        
        //continue        
    }   
    
    status = bus_deinit();
    if (status != MblError::None){
        one_set_status.set(status);
        TR_ERR("bus_deinit() failed with error %s", MblError_to_str(status));        
        //continue        
    }

    status = event_loop_deinit();
    if (status != MblError::None){
        one_set_status.set(status);
        TR_ERR("mailbox_in_.deini()) failed with error %s", MblError_to_str(status));
        //continue        
    }

    status = event_manager_.deinit();
    if (status != MblError::None){
        one_set_status.set(status);
        TR_ERR("event_manager_.deinit() failed with error %s", MblError_to_str(status));
        //continue        
    }

    // TODO - make sure we clean all references to messages, timers etc. and unref all all sources

    DBusService_deinit();

    state_.set(DBusAdapterState::UNINITALIZED);
    if (MblError::None == one_set_status.get()){
        TR_INFO("Deinit finished with SUCCESS!");
    }
    return one_set_status.get();
}


MblError DBusAdapterImpl::event_loop_run(MblError &stop_status) 
{
    TR_DEBUG("Enter - Start running!");

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
    TR_DEBUG("Enter");
    MblError status = MblError::Unknown;

    if (state_.is_not_equal(DBusAdapterState::INITALIZED)){
        TR_ERR("Unexpected state (expected %s), returning error %s", 
            state_.to_string(), MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }
        
    status = event_loop_run(stop_status);    
    if (status != MblError::None){
        TR_ERR("event_loop_run() failed with error %s", MblError_to_str(status));
        return status;
    }
    state_.set(DBusAdapterState::INITALIZED);

    return MblError::None;
}

MblError DBusAdapterImpl::stop(MblError stop_status)
{
    TR_DEBUG("Enter");
    MblError status = MblError::Unknown;

    if (state_.is_equal(DBusAdapterState::UNINITALIZED)){
        TR_ERR("Unexpected state (expected %s), returning error %s", 
            state_.to_string(), MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }

    if (pthread_equal(pthread_self(), initializer_thread_id_) != 0){
        // This section is for self exit request, use event_loop_request_stop
        // current thread id ==  initializer_thread_id_
        int r = event_loop_request_stop(stop_status);
        if (r < 0){
            TR_ERR("event_loop_request_stop failed with error r=%d (%s) - returning %s",
                r, strerror(r), MblError_to_str(MblError::DBA_SdEventCallFailure));
            status = MblError::DBA_SdEventCallFailure;
            //continue
        }
        else {
            TR_INFO("Sent self request to exit sd-event loop!");
        }
    }
    else {
        // This section is for external threads exit requests - send EXIT message to mailbox_in_
        // Thread shouldn't block here, but we still supply a maximum timeout of 
        // MSG_SEND_ASYNC_TIMEOUT_MILLISECONDS
        MailboxMsg::MsgPayload payload;
        payload.exit_.stop_status = stop_status;
        MailboxMsg msg(MailboxMsg::MsgType::EXIT, payload, sizeof(payload.exit_));        

        status = mailbox_in_.send_msg(msg);
        if (status != MblError::None){
            TR_ERR("mailbox_in_.send_msg failed with error %s", MblError_to_str(status));
            status = MblError::DBA_SdEventCallFailure;
            //continue
        }
        else {
            TR_INFO("Sent request to stop CCRB thread inside sd-event loop!");
        }
    }

    return status;
}

MblError DBusAdapterImpl::handle_ccrb_RegisterResources_status_update(
    const uintptr_t ipc_request_handle, 
    const CloudConnectStatus reg_status)
{
    assert(ipc_request_handle != 0);
    //TODO - IMPLEMENT, remove all UNUSED
    UNUSED(ipc_request_handle);
    UNUSED(reg_status);
    TR_DEBUG("Enter");   
    assert(0);

    if (state_.is_not_equal(DBusAdapterState::RUNNING)){
        TR_ERR("Unexpected state (expected %s), returning error %s", 
            state_.to_string(), MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }
    return MblError::None;
}

MblError DBusAdapterImpl::handle_ccrb_DeregisterResources_status_update(
    const uintptr_t ipc_request_handle, 
    const CloudConnectStatus dereg_status)
{
    //TODO - IMPLEMENT, remove all UNUSED
    assert(ipc_request_handle != 0);
    UNUSED(ipc_request_handle);
    UNUSED(dereg_status);
    TR_DEBUG("Enter");
    assert(0);

    if (state_.is_not_equal(DBusAdapterState::RUNNING)){
        TR_ERR("Unexpected state (expected %s), returning error %s", 
            state_.to_string(), MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }
    return MblError::None;
}

MblError DBusAdapterImpl::handle_ccrb_AddResourceInstances_status_update(
    const uintptr_t ipc_request_handle, 
    const CloudConnectStatus add_status)
{
    assert(ipc_request_handle != 0);
    //TODO - IMPLEMENT, remove all UNUSED
    UNUSED(ipc_request_handle);
    UNUSED(add_status);    
    TR_DEBUG("Enter");
    assert(0);

    if (state_.is_not_equal(DBusAdapterState::RUNNING)){
        TR_ERR("Unexpected state (expected %s), returning error %s", 
            state_.to_string(), MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }
    return MblError::None;
}

MblError DBusAdapterImpl::handle_ccrb_RemoveResourceInstances_status_update(
    const uintptr_t ipc_request_handle, 
    const CloudConnectStatus remove_status)
{
    assert(ipc_request_handle != 0);
    //TODO - IMPLEMENT, remove all UNUSED
    UNUSED(ipc_request_handle);
    UNUSED(remove_status);
    TR_DEBUG("Enter");
    assert(0);

    if (state_.is_not_equal(DBusAdapterState::RUNNING)){
        TR_ERR("Unexpected state (expected %s), returning error %s", 
            state_.to_string(), MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }
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
        default: return "UNKNOWN SD_BUS MESSAGE TYPE!";
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
            return "UNKNOWN STATE"; 
    }
}

void DBusAdapterImpl::State::set(eState new_state) 
{
    if (current_ != new_state){
        current_ = new_state;
        TR_INFO("New adapter state %s", to_string());
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

MblError DBusAdapterImpl::send_event_immediate(
    SelfEvent::EventData data,
    unsigned long data_length,
    SelfEvent::EventType event_type,        
    SelfEventCallback callback,
    uint64_t &out_event_id,
    const char *description)
{
    TR_DEBUG("Enter");
    assert(callback);
    if (pthread_equal(pthread_self(), initializer_thread_id_) == 0){
         TR_ERR("Only CCRB thread should call thisfunction - returning %s",
            MblError_to_str(MblError::DBA_ForbiddenCall));
        return MblError::DBA_ForbiddenCall;
    }
    return event_manager_.send_event_immediate(
        data, data_length, event_type, callback, out_event_id, description);    
}
    
} // namespace mbl {