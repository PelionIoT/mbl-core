/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cassert>
#include <pthread.h>
#include <unistd.h>
#include <set>

#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>

//#include "mbed-trace/mbed_trace.h"  // FIXME : uncomment
#include "DBusAdapter.h"
#include "DBusMailboxMsg.h"
#include "DBusAdapterMailbox.h"
#include "DBusAdapterService.h"

//TODO include also upper layer header

#define TRACE_GROUP "ccrb-dbus"

// sd-bus vtable object, implements the com.mbed.Cloud.Connect1 interface
#define DBUS_CLOUD_SERVICE_NAME                 "com.mbed.Cloud"
#define DBUS_CLOUD_CONNECT_INTERFACE_NAME       "com.mbed.Cloud.Connect1"
#define DBUS_CLOUD_CONNECT_OBJECT_PATH          "/com/mbed/Cloud/Connect1"

#define RETURN_0_ON_SUCCESS(retval) (((retval) >= 0) ? 0 : retval)

namespace mbl {

// TODO : seperate DBusAdapter and DBusAdapterImpl


//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////// DBusAdapterImpl /////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////


class DBusAdapter::DBusAdapterImpl
{
public:
    DBusAdapterImpl();
    ~DBusAdapterImpl();

    MblError init();
    MblError deinit();
  
private:
      //wait no more than 10 milliseconds in order to send an asynchronus message of any type
    static const uint32_t  MSG_SEND_ASYNC_TIMEOUT_MILLISECONDS = 10;

    enum class Status {
        NON_INITALIZED, 
        INITALIZED, 
        RUNNING}  status_ = Status::NON_INITALIZED;

    /*
     callbacks + handle functions
    Callbacks are static and pipe the call to the actual object implementation member function
    */
    static int incoming_bus_message_callback(
       sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
    int incoming_bus_message_callback_impl(
        sd_bus_message *m, sd_bus_error *ret_error);
    
    static int name_owner_changed_match_callback(
        sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
    int name_owner_changed_match_callback_impl(
        sd_bus_message *m, sd_bus_error *ret_error);

    static int incoming_mailbox_message_callback(
        sd_event_source *s, 
        int fd,
 	    uint32_t revents,
        void *userdata);
    int incoming_mailbox_message_callback_impl(
        sd_event_source *s, 
        int fd, 
        uint32_t revents);

    int process_incoming_message_RegisterResources(
        const sd_bus_message *m, 
        const char *appl_resource_definition_json);

    int process_incoming_message_DeregisterResources(
        const sd_bus_message *m, 
        const char *access_token);

    //TODO : uncomment and find solution to initializing for gtest without ResourceBroker
    // this class must have a reference that should be always valid to the CCRB instance. 
    // reference class member satisfy this condition.   
    //ResourceBroker &ccrb_;    
        
    //DBusAdapterCallbacks   lower_level_callbacks_;    
    DBusAdapterMailbox     mailbox_;    // TODO - empty on deinit
    pthread_t              initializer_thread_id_;

    MblError bus_init();
    MblError bus_deinit();

    MblError event_loop_init();
    MblError event_loop_deinit();
    MblError event_loop_run();
    MblError event_loop_request_stop(int32_t exit_code);


    // A set which stores upper-layer-asynchronous bus request handles (e.g incoming method requests)
    // Keep here any handle which needs tracking - if the request is not fullfiled during the event dispatching
    // it is kept inside this set
    // TODO : consider adding timestamp to avoid container explosion + periodic cleanup
    std::set<const sd_bus_message*>    pending_messages_;

    // D-Bus
    sd_event                *event_loop_handle_;
    sd_bus                  *connection_handle_;
    sd_bus_slot             *connection_slot_;         // TODO : needed?
    sd_event_source         *event_source_pipe_;  
    const char              *unique_name_;
    const char              *service_name_;       
};


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
    
    r = sd_bus_open_user(&connection_handle_);    
    if (r < 0){
        return MblError::DBusErr_Temporary;
    }
    if (NULL == connection_handle_){
        return MblError::DBusErr_Temporary;
    }
    
    const sd_bus_vtable* table = DBusAdapterService_get_service_vtable();
    if (nullptr == table){
        return MblError::DBusErr_Temporary;
    }
    // Install the object
    r = sd_bus_add_object_vtable(connection_handle_,
                                 &connection_slot_,
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
    
    r = sd_bus_add_match(
        connection_handle_,
        NULL, 
        "type='signal',interface='org.freedesktop.DBus',member='NameOwnerChanged'",
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

    if (service_name_){
        int r = sd_bus_release_name(connection_handle_, DBUS_CLOUD_SERVICE_NAME);
        if (r < 0){
            // TODO : print error
        }
        else {
            service_name_ = nullptr;
        }        
    }
    if (connection_slot_){
        sd_bus_slot_unref(connection_slot_);
        connection_slot_ = nullptr;
    }
    if (connection_handle_){
        sd_bus_unref(connection_handle_);
        connection_handle_ = nullptr;
    }
    return MblError::None;
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
        &event_source_pipe_, 
        mailbox_.get_pipefd_read(), 
        EPOLLIN, 
        DBusAdapter::DBusAdapterImpl::incoming_mailbox_message_callback,
        this);
    if (r < 0){
        return MblError::DBusErr_Temporary;
    }

    r = sd_bus_attach_event(connection_handle_, event_loop_handle_, SD_EVENT_PRIORITY_NORMAL);
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

MblError DBusAdapter::DBusAdapterImpl::event_loop_run() 
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    int r = -1;

    r = sd_event_loop(event_loop_handle_);
    if (r < 0){
        return MblError::DBusErr_Temporary;
    }
    return MblError::None;
}

MblError DBusAdapter::DBusAdapterImpl::event_loop_request_stop(int exit_code) 
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    int r;

    //only my thread id is allowd to call this one, check no other thread
    //is using this function in order to prevent confusion.
    //any other thread should send a DBUS_ADAPTER_MSG_EXIT message via mailbox
    if (pthread_self() != initializer_thread_id_){
        return MblError::DBusErr_Temporary;
    }
    r = sd_event_exit(event_loop_handle_, exit_code);
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
    return RETURN_0_ON_SUCCESS(r);
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

    if (revents & EPOLLIN == 0){
         // TODO : print , fatal error. what to do?
        return -1;
    }
    if (s != event_source_pipe_){
        // TODO : print , fatal error. what to do?
        return -1;
    }
    if (fd != mailbox_.get_pipefd_read()){
        return r;
    }
    status = mailbox_.receive_msg(msg, DBUS_MAILBOX_TIMEOUT_MILLISECONDS);
    if (status != MblError::None) {
        return r;
    }

    switch (msg.type_)
    {
        case mbl::DBusMailboxMsg::MsgType::EXIT:
            if (msg.payload_len_ != sizeof(mbl::DBusMailboxMsg::Msg_exit_)){
                break;
            }
            r = event_loop_request_stop(msg.payload_.exit.exit_code);
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

    if (Status::NON_INITALIZED != status_){
        return Error::DBusErr_Temporary;
    }

    status = mailbox_.init();
    if (status != Error::None){
        return status;
    }

    status = bus_init();
     if (status != Error::None){
        return status;
    }

    status = event_loop_init();
    if (status != Error::None){
        return status;
    }

    initializer_thread_id_ = pthread_self();
    status_ = Status::INITALIZED;
    return Error::None;
}



//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////// DBusAdapter /////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

/*
For holding std::unique_ptr<DBusAdapterImpl*>, need to allow unique_ptr to recognize DBusAdapterImpl as a complete type.
std::unique_ptr checks  in its destructor if the definition of the type is visible before calling delete.
DBusAdapterImpl is forward decleared in header (that a call to delete is well-formed);
std::unique_ptr will refuse to compile and to call delete if the type is only forward declared.
Use the default implentation for the destructor after the definition of DBusAdapterImpl :
*/
DBusAdapter::~DBusAdapter() = default;


//TODO  delete - temporary ctor
DBusAdapter::DBusAdapter() : impl_(new DBusAdapterImpl)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

//TODO  uncomment and solve gtest issue for unit tests
/*
DBusAdapter::DBusAdapter(ResourceBroker &ccrb) , mpl_(new DBusAdapterImpl):
    ccrb_(ccrb)
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    // set callbacks to DBUS lower layer    
    lower_level_callbacks_.register_resources_async_callback = register_resources_async_callback;
    lower_level_callbacks_.deregister_resources_async_callback = deregister_resources_async_callback;
}
*/



MblError DBusAdapter::init()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    MblError status = impl_->init();
    if (status != MblError::None){
        impl_->deinit();
    }
    return status;
}






} // namespace mbl {

/*


int DBusAdapterLowLevel_init(const DBusAdapterCallbacks *adapter_callbacks, void *userdata)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    int r;

    memset(&ctx_, 0, sizeof(ctx_));
    ctx_.initializer_thread_id_ = pthread_self();
    r = DBusAdapterBusService_init(adapter_callbacks, userdata);
    if (r < 0){
        return r;
    }
    r = DBusAdapterEventLoop_init();
    if (r < 0){
        return r;
    } 

    r = sd_bus_attach_event(ctx_.connection_handle, ctx_.event_loop_handle, SD_EVENT_PRIORITY_NORMAL);
    if (r < 0){
        return r;
    }
    return 0;
}


int DBusAdapterLowLevel_deinit()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    int r1, r2, r3;
    
    // best effort
    r1 = sd_bus_detach_event(ctx_.connection_handle);
    r2 = DBusAdapterBusService_deinit();
    r3 = DBusAdapterEventLoop_deinit();

    memset(&ctx_, 0, sizeof(ctx_));
    return (r1 < 0) ? r1 : (r2 < 0) ? r2 : (r3 < 0) ? r3 : 0;
}






//////////////////////////////////////////////////////
//  Upper Layer calls 
//////////////////////////////////////////////////////

/*




/*


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

    // TODO - reember to unref all
    //for (auto &handle : bus_request_handles_){
        //sd_bus_message_unref((sd_bus_message*)handle);
    //}
    status_ = DBusAdapter::Status::NON_INITALIZED;
    return Error::None;
}

MblError DBusAdapter::run()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    if (DBusAdapter::Status::INITALIZED != status_){
        return Error::DBusErr_Temporary;
    }
        
    status_ = DBusAdapter::Status::RUNNING;
    int r = DBusAdapterLowLevel_event_loop_run();    
    status_ = DBusAdapter::Status::INITALIZED;

    return (r == 0) ? Error::None : Error::DBusErr_Temporary;
}

MblError DBusAdapter::stop()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    MblError status;
    int exit_code = 0; //set 0 as exit code for now
    if (DBusAdapter::Status::RUNNING != status_){
        return Error::DBusErr_Temporary;
    }

    if (pthread_self() == master_thread_id_){
        int r = DBusAdapterLowLevel_event_loop_request_stop(exit_code);
        if (r < 0){
            // print error
        }
        status = (r == 0) ? Error::None : Error::DBusErr_Temporary;
    }
    else
    {
        DBusMailboxMsg msg;
        msg.type_ = mbl::DBusMailboxMsg::MsgType::EXIT;
        msg.payload_len_ = sizeof(mbl::DBusMailboxMsg::Msg_exit_);
        msg.payload_.exit.exit_code = exit_code;
        status = mailbox_.send_msg(msg, MSG_SEND_ASYNC_TIMEOUT_MILLISECONDS);
        if (status != MblError::None){
            //print something
        }
    }
    return status;
}


MblError DBusAdapter::update_registration_status(
    const uintptr_t bus_request_handle, 
    const std::string &access_token,
    const CloudConnectStatus status)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    
    if (DBusAdapter::Status::RUNNING != status_){
        return Error::DBusErr_Temporary;
    }
    
    return Error::None;
}

MblError DBusAdapter::update_deregistration_status(
    const uintptr_t bus_request_handle, 
    const CloudConnectStatus status)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(0);
    if (DBusAdapter::Status::RUNNING != status_){
        return Error::DBusErr_Temporary;
    }
    return Error::None;
}

// TODO - IMPLEMENT
MblError DBusAdapter::update_add_resource_instance_status(
    const uintptr_t , 
    const CloudConnectStatus )
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(0);
    if (DBusAdapter::Status::RUNNING != status_){
        return Error::DBusErr_Temporary;
    }
    return Error::None;
}

// TODO - IMPLEMENT
MblError DBusAdapter::update_remove_resource_instance_status(
    const uintptr_t , 
    const CloudConnectStatus )
{    
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(0);
    if (DBusAdapter::Status::RUNNING != status_){
        return Error::DBusErr_Temporary;
    }
    return Error::None;
}



*/