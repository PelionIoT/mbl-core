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

#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>

#include "MblSdbusAdaptor.h"

// FIXME: uncomment later
//#include "mbed-trace/mbed_trace.h"

// FIXME : remove later
#define tr_info(s)
#define tr_debug(s)
#define tr_error(s)

#define TRACE_GROUP "ccrb-dbus"



typedef struct MblSdbus
{
    sd_bus      *bus;
    sd_bus_slot *bus_slot;         // TODO : needed?
    const char  *unique_name;

    MblSdbusCallbacks callbacks;
}MblSdbus;

typedef struct MblSdEventLoop 
{
    sd_event        *ev_loop;
}MblSdEventLoop;

typedef struct MblSdContext 
{
    struct MblSdEventLoop   sdev_loop;
    struct MblSdbus         sdbus;
}MblSdContext;

static MblSdContext ctx;

// sd-bus vtable object, implements the com.mbed.Cloud.Connect1 interface
#define DBUS_CLOUD_SERVICE_NAME                 "com.mbed.Cloud"
#define DBUS_CLOUD_CONNECT_INTERFACE_NAME       "com.mbed.Cloud.Connect1"
#define DBUS_CLOUD_CONNECT_OBJECT_PATH          "/com/mbed/Cloud/Connect1"

static int register_resources_callback(
    sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    tr_debug(__PRETTY_FUNCTION__);
    MblSdContext *ctx = (MblSdContext*)userdata;
    const char *str;
    CCRBStatus ccrb_status;

    // TODO:
    // validate app registered expected interface on bus?

    int r = sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &str);
    ctx->sdbus.callbacks.register_resources_callback(str, &ccrb_status);
 
    return 0;
}

static int deregister_resources_callback(
    sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    tr_debug(__PRETTY_FUNCTION__);
    MblSdContext *ctx = (MblSdContext*)userdata;
    const char *str;
    CCRBStatus ccrb_status;

    int r = sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &str);
    ctx->sdbus.callbacks.deregister_resources_callback(str, &ccrb_status);
 
    return 0;
}

int name_owner_changed_match_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
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

// TODO: Move to a new file dedicated for vtable
static const sd_bus_vtable calculator_vtable[] = {
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

static int32_t SdBusAdaptor_event_loop_init(MblSdEventLoop *sd_event_loop)
{    
    tr_debug(__PRETTY_FUNCTION__);
    sd_event *loop = NULL;
    int32_t r = 0;

    r = sd_event_default(&loop);
    if (r < 0){
        goto on_failure;
    }

    sd_event_loop->ev_loop = loop;
    
    return 0;

on_failure:
    sd_event_unref(loop);
    return r;
}

static int32_t SdBusAdaptor_event_loop_finalize(MblSdEventLoop *sd_event_loop)
{
    tr_debug(__PRETTY_FUNCTION__);
    sd_event_unref(sd_event_loop->ev_loop);
}

static int32_t SdBusAdaptor_bus_init(MblSdbus *sdbus, const MblSdbusCallbacks *callbacks)
{
    tr_debug(__PRETTY_FUNCTION__);    
    int32_t r = -1;
    sd_bus *bus = NULL;
    sd_bus_slot *slot = NULL;
    const char  *unique_name = NULL;

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
                                 calculator_vtable,
                                 &ctx);
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
        &ctx);
    if (r < 0) {
        goto on_failure;
    }

    sdbus->callbacks = *callbacks;
    sdbus->bus = bus;
    sdbus->bus_slot = slot;
    sdbus->unique_name = unique_name;

    return 0;

on_failure:

    sd_bus_unref(bus);
    return r;
}

static int32_t SdBusAdaptor_bus_finalize(MblSdbus *sdbus)
{
    tr_debug(__PRETTY_FUNCTION__); 
    int32_t r = 0;
  
    sd_bus_unref(sdbus->bus);
    return r;
}

int32_t SdBusAdaptor_init(const MblSdbusCallbacks *callbacks)
{
    SdBusAdaptor_bus_init(&ctx.sdbus, callbacks);
    SdBusAdaptor_event_loop_init(&ctx.sdev_loop);
}

int32_t SdBusAdaptor_finalize()
{
    SdBusAdaptor_bus_finalize(&ctx.sdbus);
    SdBusAdaptor_event_loop_finalize(&ctx.sdev_loop);
}

int32_t SdBusAdaptor_run() 
{
    int32_t r = 0;

    r = sd_bus_attach_event(ctx.sdbus.bus, ctx.sdev_loop.ev_loop, SD_EVENT_PRIORITY_NORMAL);
    if (r < 0){
        return r;
    }
    
    r = sd_event_loop(ctx.sdev_loop.ev_loop);
    if (r < 0){
        return r;
    }

    return 0;
}