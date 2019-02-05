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
#include "DBusAdapterMailbox.h"
//TODO include also upper layer header

#define TRACE_GROUP "ccrb-dbus"

namespace mbl {

// TODO : seperate DBusAdapter and DBusAdapterImpl


//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////// DBusAdapterImpl /////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
DBusAdapter
class DBusAdapter::DBusAdapterImpl
{

public:
    DBusAdapterImpl();
    ~DBusAdapterImpl();

    /*
     callbacks + handle functions
    Callbacks are static and pipe the call to the actual object implementation member function
    */
    static int incoming_bus_message_callback(
       sd_bus_message *m, void *userdata, sd_bus_error *ret_error);

    static int received_message_on_mailbox_callback(
        const int fd,
        void *userdata);

    int handle_register_resources_message(
        const uintptr_t bus_request_handle, 
        const char *appl_resource_definition_json);

    int handle_deregister_resources_message(
        const uintptr_t bus_request_handle, 
        const char *access_token);
  
private:
      //wait no more than 10 milliseconds in order to send an asynchronus message of any type
    static const uint32_t  MSG_SEND_ASYNC_TIMEOUT_MILLISECONDS = 10;
    enum class Status {NON_INITALIZED, INITALIZED, RUNNING};

    //TODO : uncomment and find solution to initializing for gtest without ResourceBroker
    // this class must have a reference that should be always valid to the CCRB instance. 
    // reference class member satisfy this condition.   
    //ResourceBroker &ccrb_;    
        
    Status status_ = Status::NON_INITALIZED;

    //DBusAdapterCallbacks   lower_level_callbacks_;    
    DBusAdapterMailbox     mailbox_;    // TODO - empty on deinit
    pthread_t              master_thread_id_;

    MblError bus_init();
    MblError bus_deinit();
    MblError event_loop_init();
    MblError event_loop_deinit();

    // A set which stores upper-layer-asynchronous bus request handles (e.g incoming method requests)
    // Keep here any handle which needs tracking - if the request is not fullfiled during the event dispatching
    // it is kept inside this set
    // TODO : consider adding timestamp to avoid container explosion + periodic cleanup
    std::set<uintptr_t>    bus_request_handles_;
};
 

DBusAdapter::DBusAdapterImpl::DBusAdapterImpl()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

DBusAdapter::DBusAdapterImpl::~DBusAdapterImpl()
{
    tr_debug("%s", __PRETTY_FUNCTION__);   
};




//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////// DBusAdapter /////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

// MblError DBusAdapter::init()
// {
//     tr_debug("%s", __PRETTY_FUNCTION__);
//     MblError status;
//     int r;

//     if (DBusAdapter::Status::NON_INITALIZED != status_){
//         return Error::DBusErr_Temporary;
//     }
 
//     status = mailbox_.init();
//     if (status != Error::None){
//         deinit();
//         return status;
//     }

//     status = bus_init();
//      if (status != Error::None){
//         deinit();
//         return status;
//     }

//     status = event_loop_init();
//     if (status != Error::None){
//         deinit();
//         return status;
//     }

//     master_thread_id_ = pthread_self();
//     status_ = DBusAdapter::Status::INITALIZED;
//     return Error::None;
// }



DBusAdapter::~DBusAdapter() = default;

//TODO  delete - temporary ctor
DBusAdapter::DBusAdapter() : impl_(new DBusAdapterImpl)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}


 



/*
static int incoming_bus_message_callback(
    sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    assert(0);
}
*/

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

// MblError DBusAdapterImpl::bus_init()
// {
//     tr_debug("%s", __PRETTY_FUNCTION__);    
//     int32_t r = -1;
    
//     r = sd_bus_open_user(&ctx_.connection_handle);    
//     if (r < 0){
//         goto on_failure;
//     }
//     if (NULL == ctx_.connection_handle){
//         goto on_failure;
//     }

//     // Install the object
//     r = sd_bus_add_object_vtable(ctx_.connection_handle,
//                                  &ctx_.connection_slot,
//                                  DBUS_CLOUD_CONNECT_OBJECT_PATH,  
//                                  DBUS_CLOUD_CONNECT_INTERFACE_NAME,
//                                  cloud_connect_service_vtable,
//                                  &ctx_);
//     if (r < 0) {
//         goto on_failure;
//     }

//     r = sd_bus_get_unique_name(ctx_.connection_handle, &ctx_.unique_name);
//     if (r < 0) {
//         goto on_failure;
//     }

//     // Take a well-known service name DBUS_CLOUD_SERVICE_NAME so client Apps can find us
//     r = sd_bus_request_name(ctx_.connection_handle, DBUS_CLOUD_SERVICE_NAME, 0);
//     if (r < 0) {
//         goto on_failure;
//     }
//     ctx_.service_name = DBUS_CLOUD_SERVICE_NAME;
    
//     r = sd_bus_add_match(
//         ctx_.connection_handle,
//         NULL, 
//         "type='signal',interface='org.freedesktop.DBus',member='NameOwnerChanged'",
//         name_owner_changed_match_callback, 
//         &ctx_);
//     if (r < 0) {
//         goto on_failure;
//     }

//     ctx_.adapter_callbacks = *adapter_callbacks;
//     ctx_.adapter_callbacks_userdata = userdata;
//     return 0;

// on_failure:
//     DBusAdapterBusService_deinit();
//     return r;
// }

// MblError DBusAdapter::event_loop_init()
// {    
//     tr_debug("%s", __PRETTY_FUNCTION__);
//     sd_event *handle = NULL;
//     int r = 0;
    
//     r = sd_event_default(&ctx_.event_loop_handle);
//     if (r < 0){
//         goto on_failure;
//     }
    
//      // send read fd and 'this' to be invoked on message
//     r = DBusAdapterLowLevel_event_loop_add_io(mailbox_.get_pipefd_read());
//     if (r < 0){
//         deinit();
//         return status;
//     }

//     r = sd_bus_attach_event(ctx_.connection_handle, ctx_.event_loop_handle, SD_EVENT_PRIORITY_NORMAL);
//     if (r < 0){
//         return r;
//     }

//     return 0;

// on_failure:
//     DBusAdapterEventLoop_deinit();
//     return RETURN_0_ON_SUCCESS(r);
// }


} // namespace mbl {

/*


int DBusAdapterLowLevel_init(const DBusAdapterCallbacks *adapter_callbacks, void *userdata)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    int r;

    memset(&ctx_, 0, sizeof(ctx_));
    ctx_.master_thread_id = pthread_self();
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

static int DBusAdapterBusService_deinit()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    if (ctx_.service_name){
        int r = sd_bus_release_name(ctx_.connection_handle, DBUS_CLOUD_SERVICE_NAME);
        if (r < 0){
            // TODO : print error
        }
    }
    if (ctx_.connection_slot){
        sd_bus_slot_unref(ctx_.connection_slot);
    }
    if (ctx_.connection_handle){
        sd_bus_unref(ctx_.connection_handle);
    }
    return 0;
}


static int DBusAdapterEventLoop_deinit()
{    
    tr_debug("%s", __PRETTY_FUNCTION__);
    if (ctx_.event_loop_handle){
        sd_event_unref(ctx_.event_loop_handle);
    }
    return 0;
}



int DBusAdapterLowLevel_event_loop_run() 
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    int r;
    
    r = sd_event_loop(ctx_.event_loop_handle);
    if (r < 0){
        return r;
    }

    return 0;
}

int DBusAdapterLowLevel_event_loop_request_stop(int exit_code) 
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    //only my thread id is allowd to call this one, check no other thread
    //is using this function in order to prevent confusion.
    //any other thread should send a DBUS_ADAPTER_MSG_EXIT message via mailbox
    if (pthread_self() != ctx_.master_thread_id){
        return -1;
    }
    int r = sd_event_exit(ctx_.event_loop_handle, exit_code);
    return RETURN_0_ON_SUCCESS(r);
}

int DBusAdapterLowLevel_event_loop_add_io(int fd)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
       
    int r = sd_event_add_io(
        ctx_.event_loop_handle, 
        &ctx_.event_source_pipe, 
        fd, 
        EPOLLIN, 
        incoming_mailbox_message_callback,
        (void*)ctx_.adapter_callbacks_userdata);
    return RETURN_0_ON_SUCCESS(r);
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

//////////////////////////////////////////////////////
//  Callbacks from lower layer
//////////////////////////////////////////////////////
int DBusAdapter::register_resources_async_callback(
        const uintptr_t bus_request_handle, 
        const char *appl_resource_definition_json,
        void *userdata)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    DBusAdapter *adapter_ = static_cast<DBusAdapter*>(userdata);
    return adapter_->register_resources_async_callback_impl(
        bus_request_handle, appl_resource_definition_json);
}

int DBusAdapter::register_resources_async_callback_impl(
    const uintptr_t bus_request_handle, 
    const char *appl_resource_definition_json)
{
    // Register resources is an asynchronous process towards the cloud -> store handle
    if (bus_request_handles_.insert(bus_request_handle).second == false){
        return -1;
    }
    //TODO : uncomment + handle errors
    //MblError status = ccrb.register_resources(bus_request_handle, std::string(appl_resource_definition_json));    
    return 0;
}

int DBusAdapter::deregister_resources_async_callback(
    const uintptr_t bus_request_handle, 
    const char *access_token,
    void *userdata)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    DBusAdapter *adapter_ = static_cast<DBusAdapter*>(userdata);
    return adapter_->deregister_resources_async_callback_impl(bus_request_handle, access_token);
}

int DBusAdapter::deregister_resources_async_callback_impl(
    const uintptr_t bus_request_handle, 
    const char *access_token)
{
    // Register resources is an asynchronous process towards the cloud -> store handle
    if (bus_request_handles_.insert(bus_request_handle).second == false){
        return -1;
    }
    //TODO : uncomment + handle errors
    //MblError status = ccrb.deregister_resources(bus_request_handle, std::string(deregister_resources));    
    return 0;
}

int DBusAdapter::received_message_on_mailbox_callback_impl(const int fd)
{
    MblError status;
    DBusMailboxMsg msg;
    int r = -1;

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
            r = DBusAdapterLowLevel_event_loop_request_stop(msg.payload_.exit.exit_code);
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

int DBusAdapter::received_message_on_mailbox_callback(
    const int fd,
    void* userdata)
{
    // This is a static callback
    tr_debug("%s", __PRETTY_FUNCTION__);
    DBusAdapter *adapter_ = static_cast<DBusAdapter*>(userdata);
    return adapter_->received_message_on_mailbox_callback_impl(fd);
}

*/