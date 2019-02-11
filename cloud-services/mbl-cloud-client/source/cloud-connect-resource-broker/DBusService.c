/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <systemd/sd-bus.h>

#include "CloudConnectCommon_Internal.h"
#include "DBusService.h"

#define TRACE_GROUP "ccrb-dbus"

typedef struct DBusServiceContext_
{   
    IncomingDataCallback    incoming_bus_message_callback_;
}DBusServiceContext;

static DBusServiceContext ctx_ = { 0 };
static int incoming_bus_message_callback(
    sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
    
const sd_bus_vtable  cloud_connect_service_vtable[] = {
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

    // TODO - remove after discussion
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
        "DeregisterResources",
        "s", 
        "i",
        incoming_bus_message_callback,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),

    // TODO - remove after discussion
    // ==Signal== - emitted on successful resources deregistration
    //
    // Argument	    Type	Description
    // 0	        INT32	Cloud Connect Status Code
    //
    // ==Possible Cloud connect status values==
    // TBD
    SD_BUS_SIGNAL(
        "DeregisterResourcesResult",
        "i",
        0
    ),

    SD_BUS_VTABLE_END
};

static int incoming_bus_message_callback(
    sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    tr_debug("Enter");
    return ctx_.incoming_bus_message_callback_(m, userdata, ret_error);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////// API /////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// the userdata will be transferred as part of vtable object add
void DBusService_init(IncomingDataCallback callback)
{
    tr_debug("Enter");
    memset(&ctx_, 0, sizeof(ctx_));
    ctx_.incoming_bus_message_callback_ = callback;
}

void DBusService_deinit()
{   
    tr_debug("Enter");
    memset(&ctx_, 0, sizeof(ctx_));
}

const sd_bus_vtable* DBusService_get_service_vtable()
{
    return cloud_connect_service_vtable;
}
