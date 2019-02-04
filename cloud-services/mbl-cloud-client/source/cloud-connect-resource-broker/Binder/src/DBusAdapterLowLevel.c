/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>
#include <pthread.h>
#include <string.h>


#include "DBusAdapterLowLevel_internal.h"
#include "DBusAdapterLowLevel.h"

#define TRACE_GROUP "ccrb-dbus"
#define RETURN_0_ON_SUCCESS(retval) (((retval) >= 0) ? 0 : retval)


//////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Declerations + initializations ///////////////////
//////////////////////////////////////////////////////////////////////////////////
static DBusAdapterLowLevelContext ctx_ = { 0 };

static int DBusAdapterBusService_deinit();
static int DBusAdapterEventLoop_deinit();

//////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Event loop Attached Callbacks ////////////////////
//////////////////////////////////////////////////////////////////////////////////
static int incoming_mailbox_message_callback(
    sd_event_source *s, 
    int fd,
 	uint32_t revents,
    void *userdata)
{
    if (revents & EPOLLIN == 0){
         // TODO : print , fatal error. what to do?
        return -1;
    }
    if (s != ctx_.event_source_pipe){
        // TODO : print , fatal error. what to do?
        return -1;
    }
   
    int r = ctx_.adapter_callbacks.received_message_on_mailbox_callback(fd, userdata);
    if (r < 0){
         // TODO : print , fatal error. what to do?
         return r;
    }
    return 0;
}

static int name_owner_changed_match_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
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


//////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// D-BUS Service Callbacks //////////////////////////
//////////////////////////////////////////////////////////////////////////////////
int incoming_bus_message_callback(
    sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    int r;
    DBusAdapterLowLevelContext *ctx = (DBusAdapterLowLevelContext*)userdata;
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
    const char *signature = sd_bus_message_get_signature(m, true);
    if (r < 0){
        return r;
    }

    if (sd_bus_message_is_method_call(m, 0, "RegisterResources")){
        const char *json_file_data = NULL;       
        
        if (0 != strncmp(signature,"s", 1)){
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
        
        r = ctx_.adapter_callbacks.register_resources_async_callback(
            (const uintptr_t)m,
            json_file_data,
            ctx_.adapter_callbacks_userdata);
        if (r < 0){
            //TODO : print , fatal error. what to do?
            return r;
        }
        // success - increase ref
        sd_bus_message_ref(m);
    }
    else if (sd_bus_message_is_method_call(m, 0, "DeRegisterResources")) {
        sd_bus_message_ref(m);
        assert(0);
    }
    else {
        //TODO - reply with error? or just pass?
        return -1;
    }

    return 0;
}


// TODO: Move to a new file dedicated for vtable
static const sd_bus_vtable     cloud_connect_service_vtable[] = {
    SD_BUS_VTABLE_START(0),

    // ==Method== - Request to register resources supplied by a JSON file
    // 
    // ==Input==
    // Argument	    Type	Description
    // 0	        STRING	JSON file (encoded UTF-8)
    //
    // ==Output==
    // Argument	    Type	Description
    // 0	        INT32	Cloud Connect Status
    //
    // ==Possible Cloud connect status values==
    // TBD
    SD_BUS_METHOD(
        "RegisterResources",
        "s", 
        "i",
        incoming_bus_message_callback,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),

    // ==Signal== - emitted as a final result for resources registration asynchronous request
    //
    // Argument	    Type	Description
    // 0	        INT32	Cloud connect status Code
    // 1	        STRING	access-token - relevant only on successful registration
    //
    // ==Possible Cloud connect status values==
    // TBD
    SD_BUS_SIGNAL(
        "RegisterResourcesResult",
        "is",
        0
    ),

    // ==Method== - Request to de-register all previously registered resources for supplied access-token.
    //
    // ==Input==
    // Argument	    Type	Description
    // 0	        STRING	access-token
    //
    // ==Output==
    // Argument	    Type	Description
    // 0	        INT32	Cloud Connect Status
    //
    // ==Possible Cloud connect status values==
    // TBD
    SD_BUS_METHOD(
        "DeRegisterResources",
        "s", 
        "i",
        incoming_bus_message_callback,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),

    // ==Signal== - emitted on successful resources deregistration
    //
    // Argument	    Type	Description
    // 0	        INT32	Cloud Connect Status Code
    //
    // ==Possible Cloud connect status values==
    // TBD
    SD_BUS_SIGNAL(
        "DeRegisterResourcesResult",
        "i",
        0
    ),

    SD_BUS_VTABLE_END
};


static int DBusAdapterBusService_init(
    const DBusAdapterCallbacks *adapter_callbacks, 
    void *userdata)
{
    tr_debug("%s", __PRETTY_FUNCTION__);    
    int32_t r = -1;
    
    if ((NULL == adapter_callbacks) ||
        (NULL == adapter_callbacks->deregister_resources_async_callback) ||
        (NULL == adapter_callbacks->register_resources_async_callback) || 
        (NULL == adapter_callbacks->received_message_on_mailbox_callback))
    {
        goto on_failure;
    }

    r = sd_bus_open_user(&ctx_.connection_handle);    
    if (r < 0){
        goto on_failure;
    }
    if (NULL == ctx_.connection_handle){
        goto on_failure;
    }

    // Install the object
    r = sd_bus_add_object_vtable(ctx_.connection_handle,
                                 &ctx_.connection_slot,
                                 DBUS_CLOUD_CONNECT_OBJECT_PATH,  
                                 DBUS_CLOUD_CONNECT_INTERFACE_NAME,
                                 cloud_connect_service_vtable,
                                 &ctx_);
    if (r < 0) {
        goto on_failure;
    }

    r = sd_bus_get_unique_name(ctx_.connection_handle, &ctx_.unique_name);
    if (r < 0) {
        goto on_failure;
    }

    // Take a well-known service name DBUS_CLOUD_SERVICE_NAME so client Apps can find us
    r = sd_bus_request_name(ctx_.connection_handle, DBUS_CLOUD_SERVICE_NAME, 0);
    if (r < 0) {
        goto on_failure;
    }
    ctx_.service_name = DBUS_CLOUD_SERVICE_NAME;
    
    r = sd_bus_add_match(
        ctx_.connection_handle,
        NULL, 
        "type='signal',interface='org.freedesktop.DBus',member='NameOwnerChanged'",
        name_owner_changed_match_callback, 
        &ctx_);
    if (r < 0) {
        goto on_failure;
    }

    ctx_.adapter_callbacks = *adapter_callbacks;
    ctx_.adapter_callbacks_userdata = userdata;
    return 0;

on_failure:
    DBusAdapterBusService_deinit();
    return r;
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


static int DBusAdapterEventLoop_init()
{    
    tr_debug("%s", __PRETTY_FUNCTION__);
    sd_event *handle = NULL;
    int r = 0;
    
    r = sd_event_default(&ctx_.event_loop_handle);
    if (r < 0){
        goto on_failure;
    }
    
    return 0;

on_failure:
    DBusAdapterEventLoop_deinit();
    return RETURN_0_ON_SUCCESS(r);
}

static int DBusAdapterEventLoop_deinit()
{    
    tr_debug("%s", __PRETTY_FUNCTION__);
    if (ctx_.event_loop_handle){
        sd_event_unref(ctx_.event_loop_handle);
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// API //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

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

DBusAdapterLowLevelContext* DBusAdapterLowLevel_GetContext()
{
    return &ctx_;
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