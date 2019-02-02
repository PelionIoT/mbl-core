/*
 * Copyright (c) 2019 ARM Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>

#include "DBusAdapterLowLevel_internal.h"
#include "DBusAdapterLowLevel.h"

#define TRACE_GROUP "ccrb-dbus"

//////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Declerations + initializations ///////////////////
//////////////////////////////////////////////////////////////////////////////////
static DBusAdapterLowLevelContext ctx_ = { 0 };

static int DBusAdapterBusService_deinit();

//////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Event loop Attached Callbacks ////////////////////
//////////////////////////////////////////////////////////////////////////////////
static int pipe_incoming_msg_callback(
    sd_event_source *s, 
    int fd,
 	uint32_t revents,
    void *userdata)
{
    int a = 7;
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
    size_t  size;

    int r = sd_bus_message_read(m, "sss", &msg_args[0], &msg_args[1], &msg_args[2]);

    return 0;
}


//////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// D-BUS Service Callbacks //////////////////////////
//////////////////////////////////////////////////////////////////////////////////
int register_resources_callback(
    sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    DBusAdapterLowLevelContext *ctx = (DBusAdapterLowLevelContext*)userdata;
    const char *str;

    // TODO:
    // validate app registered expected interface on bus? (use sd-bus track)

    int r = sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &str);
    if (r < 0){
        return r;
    }

    //ctx->adapter_callbacks.register_resources_async_callback() ...
 
    return 0;
}

static int deregister_resources_callback(
    sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    DBusAdapterLowLevelContext *ctx = (DBusAdapterLowLevelContext*)userdata;
    int r;

    //int r = sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &str);
    //ctx->sdbus.callbacks.deregister_resources_callback(str, &ccrb_status);
 
    return 0;
}

// TODO: Move to a new file dedicated for vtable
static const sd_bus_vtable     cloud_connect_service_vtable[] = {
    SD_BUS_VTABLE_START(0),

    // This message contains JSON file with resources to be registered.
    //
    // Input:
    // Argument	    Type	Description
    // 0	        STRING	JSON file (encoded UTF-8)
    //
    // Output:
    // Argument	    Type	Description
    // 0	        INT32	CCRBStatus
    SD_BUS_METHOD(
        "RegisterResources",
        "s", 
        "u",
        register_resources_callback,
        SD_BUS_VTABLE_UNPRIVILEGED),


    // This message de-register all previously registered resources for supplied access-token.
    //
    // Input:
    // Argument	    Type	Description
    // 0	        STRING	access-token string
    //
    // Output:
    // Argument	    Type	Description
    // 0	        INT32	CCRBStatus
    SD_BUS_METHOD(
        "DeRegisterResources",
        "s", 
        "u",
        deregister_resources_callback,
        SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_VTABLE_END
};


static int DBusAdapterBusService_init(const DBusAdapterCallbacks *adapter_callbacks)
{
    tr_debug("%s", __PRETTY_FUNCTION__);    
    int32_t r = -1;
    sd_bus *bus = NULL;
    sd_bus_slot *slot = NULL;
    const char  *unique_name = NULL;

    if ((NULL == adapter_callbacks) ||
        (NULL == adapter_callbacks->deregister_resources_async_callback) ||
        (NULL == adapter_callbacks->register_resources_async_callback))
    {
        goto on_failure;
    }

    r = sd_bus_open_user(&bus);    
    if (r < 0){
        goto on_failure;
    }
    if (NULL == bus){
        goto on_failure;
    }

    // Install the object
    r = sd_bus_add_object_vtable(bus,
                                 &slot,
                                 DBUS_CLOUD_CONNECT_OBJECT_PATH,  
                                 DBUS_CLOUD_CONNECT_INTERFACE_NAME,
                                 cloud_connect_service_vtable,
                                 &ctx_);
    if (r < 0) {
        goto on_failure;
    }

    r = sd_bus_get_unique_name(bus, &unique_name);
    if (r < 0) {
        goto on_failure;
    }

    // Take a well-known service name DBUS_CLOUD_SERVICE_NAME so client Apps can find us
    r = sd_bus_request_name(bus, DBUS_CLOUD_SERVICE_NAME, 0);
    if (r < 0) {
        goto on_failure;
    }
    
    r = sd_bus_add_match(
        bus, NULL, 
        "type='signal',interface='org.freedesktop.DBus',member='NameOwnerChanged'",
        name_owner_changed_match_callback, 
        &ctx_);
    if (r < 0) {
        goto on_failure;
    }

    ctx_.adapter_callbacks = *adapter_callbacks;
    ctx_.connection_handle = bus;
    ctx_.connection_slot = slot;
    ctx_.unique_name = unique_name;

    return 0;

on_failure:
    DBusAdapterBusService_deinit();
    return r;
}

static int DBusAdapterBusService_deinit()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    int r = sd_bus_release_name(ctx_.connection_handle, DBUS_CLOUD_SERVICE_NAME);
    if (r < 0){
        // TODO : print error
    }
    sd_bus_slot_unref(ctx_.connection_slot);
    sd_bus_unref(ctx_.connection_handle);
    return 0;
}


static int DBusAdapterEventLoop_init()
{    
    tr_debug("%s", __PRETTY_FUNCTION__);
    sd_event *handle = NULL;
    int r = 0;
    
    r = sd_event_default(&handle);
    if (r < 0){
        goto on_failure;
    }

    ctx_.event_loop_handle = handle;
    
    return 0;

on_failure:
    sd_event_unref(handle);
    return r;
}

static int DBusAdapterEventLoop_deinit()
{    
    tr_debug("%s", __PRETTY_FUNCTION__);
    sd_event_unref(ctx_.event_loop_handle);
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// API //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

int DBusAdapterLowLevel_init(const DBusAdapterCallbacks *adapter_callbacks)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    int r;

    memset(&ctx_, 0, sizeof(ctx_));
    r = DBusAdapterBusService_init(adapter_callbacks);
    if (r < 0){
        return r;
    }
    r = DBusAdapterEventLoop_init();
    if (r < 0){
        return r;
    } 
    return 0;
}

int DBusAdapterLowLevel_deinit()
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    // best effort
    int r1 = DBusAdapterBusService_deinit();
    int r2 = DBusAdapterEventLoop_deinit();
    memset(&ctx_, 0, sizeof(ctx_));
    return (r1 < 0) ? r1 : (r2 < 0) ? r2 : 0;
}

/*
int DBusAdapterLowLevel_attach_pipe_fd(int fd)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    sd_event_source *event_source;    
    return sd_event_add_io(ctx.sdev_loop.ev_loop, &event_source, fd, EPOLLIN, pipe_incoming_msg_callback, 0);
}
*/

DBusAdapterLowLevelContext* DBusAdapterLowLevel_GetContext()
{
    return &ctx_;
}

int DBusAdapterLowLevel_run() 
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    int r = 0;

    r = sd_bus_attach_event(ctx_.connection_handle, ctx_.event_loop_handle, SD_EVENT_PRIORITY_NORMAL);
    if (r < 0){
        return r;
    }
    
    r = sd_event_loop(ctx_.event_loop_handle);
    if (r < 0){
        return r;
    }

    return 0;
}

int DBusAdapterLowLevel_stop() 
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    int r = 0;

    return 0;
}