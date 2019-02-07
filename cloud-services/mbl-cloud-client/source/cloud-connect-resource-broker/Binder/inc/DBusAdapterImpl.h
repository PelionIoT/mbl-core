/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef _DBusAdapterImpl_h_
#define _DBusAdapterImpl_h_

#include <inttypes.h>
#include <pthread.h>
#include <set>

#include "MblError.h"
#include "DBusAdapterMailbox.h"
#include "DBusMailboxMsg.h"
#include "DBusAdapter.h"

#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>

class DBusAdapterTester;

namespace mbl {

class DBusAdapter::DBusAdapterImpl
{
friend class ::DBusAdapterTester;
public:
    DBusAdapterImpl();
    ~DBusAdapterImpl();

    MblError init();
    MblError deinit();
    MblError run(DBusAdapterStopStatus &stop_status);
    MblError stop(DBusAdapterStopStatus stop_status);
    
    MblError handle_ccrb_RegisterResources_status_update(
        const uintptr_t ipc_conn_handle, 
        const std::string &access_token,
        const CloudConnectStatus reg_status);

    MblError handle_ccrb_DeregisterResources_status_update(
        const uintptr_t ipc_conn_handle, 
        const CloudConnectStatus dereg_status);

    MblError handle_ccrb_AddResourceInstances_status_update(
        const uintptr_t ipc_conn_handle, 
        const CloudConnectStatus add_status);

    MblError handle_ccrb_RemoveResourceInstances_status_update(
        const uintptr_t ipc_conn_handle, 
        const CloudConnectStatus remove_status);

private:
      //wait no more than 10 milliseconds in order to send an asynchronus message of any type
    static const uint32_t  MSG_SEND_ASYNC_TIMEOUT_MILLISECONDS = 10;

    enum class State {
        UNINITALIZED, 
        INITALIZED,         
        RUNNING,
        }  state_ = State::UNINITALIZED;

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
        
    MblError bus_init();
    MblError bus_deinit();

    MblError event_loop_init();
    MblError event_loop_deinit();
    MblError event_loop_run(DBusAdapterStopStatus &stop_status);
    MblError event_loop_request_stop(DBusAdapterStopStatus stop_status);

    // A set which stores upper-layer-asynchronous bus request handles (e.g incoming method requests)
    // Keep here any handle which needs tracking - if the request is not fullfiled during the event dispatching
    // it is kept inside this set
    // TODO : consider adding timestamp to avoid container explosion + periodic cleanup
    std::set<const sd_bus_message*>    pending_messages_;

    // D-Bus
    sd_bus                  *connection_handle_;
    sd_bus_slot             *connection_slot_;         // TODO : needed?
    const char              *unique_name_;
    const char              *service_name_; 

    // Event loop 
    sd_event_source         *event_source_pipe_;      
    sd_event                *event_loop_handle_;

    DBusAdapterMailbox     mailbox_;    // TODO - empty on deinit
    pthread_t              initializer_thread_id_;
};

}  //namespace mbl {
#endif //_DBusAdapterImpl_h_