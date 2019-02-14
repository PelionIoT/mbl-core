/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <systemd/sd-bus.h>

#include "CloudConnectCommon_Internal.h"
#include "DBusService.h"

#define TRACE_GROUP "ccrb-dbus"

/*
sd-bus does not support Natavely compilation
*/

typedef struct DBusServiceContext_
{   
    IncomingDataCallback    incoming_bus_message_callback_;
}DBusServiceContext;

static DBusServiceContext ctx_ = { 0 };
static int incoming_bus_message_callback(
    sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
    
const sd_bus_vtable  cloud_connect_service_vtable[] = {
    SD_BUS_VTABLE_START(0),

    // TODO: Consider removing Cloud Connect Status that is returned in the "method reply" 
    //       in to the following functions: RegisterResources, DeregisterResources, 
    //       AddResourceInstances, RemoveResourceInstances is placeholder. 
    //       If unneeded, JUST BEFORE publishing to a master consider to remove this status.

    // TODO: Add readable explanation about each Cloud Client Status or Error that 
    //       can be returned by any function or signal defined in this table:
    // Types:
    // CloudConnectStatus (Cloud Connect Status) :
    // TBD
    // CloudConnectError (Cloud Connect Error)
    // TBD


    // com.mbed.Cloud.Connect1.RegisterResources
    //
    // As a Method :
    // UINT32, STRING RegisterResources(STRING json)
    //
    // Description :
    // Asynchronous request to register LwM2M resources supplied by a JSON string.
    // When the registration is finished, the operation status will be sent by 
    // RegisterResourcesStatus signal.
    // ==Input==
    // Argument	    Type    Description
    // 0            STRING  JSON string (encoded UTF-8)
    //
    // ==Output==
    // Argument	    Type    Description
    // 0            UINT32  Cloud Connect Status of an attempt to start a registration. 
    // 1            STRING  Access token.
    //
    // ==Possible Cloud Connect Status values==
    // TBD
    //
    // ==Error Reply==
    // Argument     Type    Description
    // 0            STRING Error description
    // 1            UINT32 Cloud Connect Error
    // ==Possible Cloud Connect Error values==
    // TBD
    SD_BUS_METHOD(
        "RegisterResources",
        "s", 
        "us",
        incoming_bus_message_callback,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),

    // com.mbed.Cloud.Connect1.RegisterResourcesStatus
    //
    // As a Signal :
    // RegisterResourcesStatus(UINT32 status)
    // Emitted when the RegisterResources asynchronous request is finished in 
    // the Pelion.
    //
    // Argument	    Type	Description
    // 0	        UINT32  Cloud Connect Status of the RegisterResources.
    //
    // ==Possible Cloud Connect Status values==
    // TBD
    //
    // ==Possible Cloud Connect Error values==
    // TBD
    SD_BUS_SIGNAL(
        "RegisterResourcesStatus",
        "u",
        0
    ),

    // com.mbed.Cloud.Connect1.DeregisterResources
    //
    // As a Method :
    // UINT32 DeregisterResources(STRING access_token)
    //
    // Description :
    // Asynchronous request to deregister all previously registered LwM2M resources for supplied 
    // access-token. When the deregistration is finished, the operation status will be sent by 
    // DeregisterResourcesStatus signal. 
    // If the RegisterResourcesStatus was not signalled after the RegisterResources method was 
    // called, this method will gracefully finish registration attempt that was started. 
    //
    // ==Input==
    // Argument	    Type	Description
    // 0	        STRING	Access token
    //
    // ==Output==
    // Argument	    Type    Description
    // 0            UINT32  Cloud Connect Status of an attempt to start deregistration. 
    //
    // ==Possible Cloud Connect Status values==
    // TBD
    //
    // ==Error Reply==
    // Argument     Type    Description
    // 0            STRING Error description
    // 1            UINT32 Cloud Connect Error
    // ==Possible Cloud Connect Error values==
    // TBD
    SD_BUS_METHOD(
        "DeregisterResources",
        "s", 
        "u",
        incoming_bus_message_callback,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),

    // com.mbed.Cloud.Connect1.DeregisterResourcesStatus
    //
    // As a Signal :
    // DeregisterResourcesStatus(UINT32 status)
    // Emitted when the DeregisterResources asynchronous request is finished in 
    // the Pelion.
    //
    // Argument	    Type	Description
    // 0	        UINT32  Cloud Connect Status of the DeregisterResources.
    //
    // ==Possible Cloud Connect Status values==
    // TBD
    //
    // ==Possible Cloud Connect Error values==
    // TBD
    SD_BUS_SIGNAL(
        "DeregisterResourcesStatus",
        "u",
        0
    ),


    // com.mbed.Cloud.Connect1.AddResourceInstances
    //
    // As a Method :
    // UINT32 AddResourceInstances(STRING access_token, 
    //                             STRING resource_path, 
    //                             ARRAY_of_UINT16 instance_ids)
    //
    // Description :
    // Asynchronous request to add LwM2M resource instances to the specific resource.
    // When the addition operation is finished, the status will be sent by 
    // AddResourceInstancesStatus signal.
    //
    // ==Input==
    // Argument	    Type    Description
    // 0	        STRING  Access token
    // 1	        STRING  path of the resource to which instances should be added.
    // 2	        ARRAY_of_UINT16 instance ids array. Each instance id is an id of 
    //              the resource instance that should be added to the given resource.  
    //
    // ==Output==
    // Argument	    Type    Description
    // 0            UINT32  Cloud Connect Status of an attempt to add resource 
    //                      instances.       
    //
    // ==Possible Cloud Connect Status values==
    // TBD
    //
    // ==Error Reply==
    // Argument     Type    Description
    // 0            STRING Error description
    // 1            UINT32 Cloud Connect Error
    // ==Possible Cloud Connect Error values==
    // TBD
    SD_BUS_METHOD(
        "AddResourceInstances",
        "ssaq", 
        "u",
        incoming_bus_message_callback,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),

    // com.mbed.Cloud.Connect1.AddResourceInstancesStatus
    //
    // As a Signal :
    // AddResourceInstancesStatus(UINT32 status)
    // Emitted when the AddResourceInstances asynchronous request is finished in 
    // the Pelion.
    //
    // Argument	    Type	Description
    // 0	        UINT32  Cloud Connect Status of the AddResourceInstances.
    //
    // ==Possible Cloud Connect Status values==
    // TBD
    //
    // ==Possible Cloud Connect Error values==
    // TBD
    SD_BUS_SIGNAL(
        "AddResourceInstancesStatus",
        "u",
        0
    ),

    // com.mbed.Cloud.Connect1.RemoveResourceInstances
    //
    // As a Method :
    // UINT32 RemoveResourceInstances(STRING access_token, 
    //                             STRING resource_path, 
    //                             ARRAY_of_UINT16 instance_ids)
    //
    // Description :
    // Request  
    // Asynchronous request to remove resource instances from the specific resource.
    // When the removal operation is finished, the status will be sent by 
    // RemoveResourceInstancesStatus signal.
    //
    // ==Input==
    // Argument	    Type    Description
    // 0	        STRING  Access token
    // 1	        STRING  path of the resource from which instances should be removed.
    // 2	        ARRAY_of_UINT16 instance ids array. Each instance id is an id of 
    //              the resource instance that should be removed from the given resource.  
    //
    // ==Output==
    // Argument	    Type    Description
    // 0            UINT32  Cloud Connect Status of an attempt to remove resource 
    //                      instances. 
    //
    // ==Possible Cloud Connect Status values==
    // TBD
    //
    // ==Error Reply==
    // Argument     Type    Description
    // 0            STRING Error description
    // 1            UINT32 Cloud Connect Error
    // ==Possible Cloud Connect Error values==
    // TBD
    SD_BUS_METHOD(
        "RemoveResourceInstances",
        "ssaq", 
        "u",
        incoming_bus_message_callback,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),

    // com.mbed.Cloud.Connect1.RemoveResourceInstancesStatus
    //
    // As a Signal :
    // RemoveResourceInstancesStatus(UINT32 status)
    // Emitted when the RemoveResourceInstances asynchronous request is finished in 
    // the Pelion.
    //
    // Argument	    Type	Description
    // 0	        UINT32  Cloud Connect Status of the RemoveResourceInstances.
    //
    // ==Possible Cloud Connect Status values==
    // TBD
    //
    // ==Possible Cloud Connect Error values==
    // TBD
    SD_BUS_SIGNAL(
        "RemoveResourceInstancesStatus",
        "u",
        0
    ),


    // com.mbed.Cloud.Connect1.SetResourcesValues
    //
    // As a Method :
    // ARRAY_of_UINT32 SetResourcesValues(STRING access_token,
    //                                    ARRAY_of_STRUCTS(STRING,UINT8,VARIANT) set_operation_input)
    //
    // Description :
    // Request to set resources values for multiple resources. 
    //
    // ==Input==
    // Argument	    Type                                   Description
    // 0	        STRING                                 access-token
    // 1	        ARRAY_of_STRUCTS(STRING,UINT8,VARIANT) array of structs that contains set operation 
    //                                                     input. Each struct in the array contains:
    //                                                     - path of the resource (STRING)
    //                                                     - resource data type (UINT8)
    //                                                     - resource value (VARIANT)
    //
    // ==Output==
    // Argument	    Type            Description
    // 0            ARRAY_of_UINT32 Cloud Connect Status array. Each entry [i] is the status of 
    //                              the set operation parameter at index [i] in the input array. 
    //
    // ==Error Reply==
    // Argument     Type    Description
    // 0            STRING Error description
    // 1            UINT32 Cloud Connect Error
    // ==Possible Cloud Connect Error values==
    // TBD
    SD_BUS_METHOD(
        "SetResourcesValues",
        "sa(syv)", 
        "au",
        incoming_bus_message_callback,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),

    // com.mbed.Cloud.Connect1.GetResourcesValues
    //
    // As a Method :
    // ARRAY_of_STRUCTS(UINT32,UINT8,VARIANT) GetResourcesValues(STRING access_token,
    //                                                           ARRAY_of_STRUCTS(STRING,UINT8) get_operation_input)
    //
    // Description :
    // Request to get resources values from multiple resources. 
    //
    // ==Input==
    // Argument	    Type                           Description
    // 0	        STRING                         access-token
    // 1	        ARRAY_of_STRUCTS(STRING,UINT8) array of structs that contains get operation 
    //                                             parameters. Each struct in the array contains:
    //                                             - path of the resource (STRING)
    //                                             - type of the resource value (UINT8)
    //
    // ==Output==
    // Argument	    Type            Description
    // 0	        ARRAY_of_STRUCTS(UINT32,UINT8,VARIANT) array of structs that contains get operation 
    //                                              output for each entry in the input array. 
    //                                              Each struct contains:
    //                                              - get operation for resource [i] value data status (UINT32)
    //                                              - resource [i] data type (UINT8). Valid only if the status 
    //                                                of the get operation is SUCCESS.
    //                                              - resource [i] value (VARIANT). Valid only if the status 
    //                                                of the get operation is SUCCESS.
    //
    // ==Error Reply==
    // Argument     Type    Description
    // 0            STRING Error description
    // 1            UINT32 Cloud Connect Error
    // ==Possible Cloud Connect Error values==
    // TBD
    SD_BUS_METHOD(
        "GetResourcesValues",
        "sa(sy)", 
        "a(uyv)",
        incoming_bus_message_callback,
        SD_BUS_VTABLE_UNPRIVILEGED
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
