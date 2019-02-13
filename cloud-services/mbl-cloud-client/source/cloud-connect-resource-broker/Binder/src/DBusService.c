/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <systemd/sd-bus.h>

#include "DBusService.h"

typedef struct DBusServiceContext_
{   
    IncomingDataCallback    incoming_bus_message_callback_;
}DBusServiceContext;

static DBusServiceContext ctx_ = { 0 };
static int incoming_bus_message_callback(
    sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
    
const sd_bus_vtable  cloud_connect_service_vtable[] = {
    SD_BUS_VTABLE_START(0),

    // com.mbed.Cloud.Connect1.RegisterResources
    //
    // As a Method :
    // UINT32, STRING RegisterResources(STRING json_file)
    //
    // Description :
    // Asynchronous request to register resources supplied by a JSON file.
    // The final registration status will be  sent by RegisterResourcesStatus signal.
    // ==Input==
    // Argument	    Type    Description
    // 0            STRING  JSON file (encoded UTF-8)
    //
    // ==Output==
    // Argument	    Type    Description
    // 0            UINT32  Temporary Cloud Connect Status of an attempt to start registration. 
    //                      The final registration status will be signalled by 
    //                      RegisterResourcesStatus.
    // 1            STRING  access token. this paramter is valid only if Cloud Connect 
    //                      Status was success.
    //
    // ==Possible Cloud Connect Status values==
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
    // Emitted as a final status for RegisterResources asynchronous request.
    // This signal notifies an application that the process of regestering resources 
    // in the Pelion is finished.
    //
    // Argument	    Type	Description
    // 0	        UINT32  Final Cloud Connect Status of the RegisterResources.
    //
    // ==Possible Cloud Connect Status values==
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
    // Asynchronous request to deregister all previously registered resources for supplied 
    // access-token. The final deregistration status will be sent by DeregisterResourcesStatus 
    // signal. 
    // If the resources were not registered (RegisterResourcesStatus was not signalled after 
    // RegisterResources method), this method will gracefully finish registration process
    // that was started (when RegisterResources was called). 
    //
    // ==Input==
    // Argument	    Type	Description
    // 0	        STRING	access-token
    //
    // ==Output==
    // Argument	    Type    Description
    // 0            UINT32  Temporary Cloud Connect Status of an attempt to start deregistration. 
    //                      The final deregistration status will be signalled by 
    //                      DeregisterResourcesStatus.  
    //
    // ==Possible Cloud Connect Status values==
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
    // Emitted as a final status for DeregisterResources asynchronous request.
    // This signal notifies an application that the process of deregestering resources 
    // in the Pelion is finished.  
    //
    // Argument	    Type	Description
    // 0	        UINT32	Final Cloud Connect Status of the DeregisterResources.
    //
    // ==Possible Cloud Connect Status values==
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
    // Request  
    // Asynchronous request to add resource instances to the specific resource.
    // The final status of the operation will be sent by AddResourceInstancesStatus signal.
    //
    // ==Input==
    // Argument	    Type    Description
    // 0	        STRING  access-token
    // 1	        STRING  path of the resource to which instances should be added.
    // 2	        ARRAY_of_UINT16 instance ids array. Each instance id is an id of 
    //              the resource instance that should be added to the given resource.  
    //
    // ==Output==
    // Argument	    Type    Description
    // 0            UINT32  Temporary Cloud Connect Status of an attempt to add resource 
    //                      instances. The final status will be signalled by 
    //                      AddResourceInstancesStatus.      
    //
    // ==Possible Cloud Connect Status values==
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
    // Emitted as a final status for AddResourceInstances asynchronous request.
    // This signal notifies an application that the process of adding resource 
    // instances is finished.  
    //
    // Argument	    Type	Description
    // 0	        UINT32	Final Cloud Connect Status of the AddResourceInstances.
    //
    // ==Possible Cloud Connect Status values==
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
    // The final status of the operation will be sent by RemoveResourceInstancesStatus signal.
    //
    // ==Input==
    // Argument	    Type    Description
    // 0	        STRING  access-token
    // 1	        STRING  path of the resource from which instances should be removed.
    // 2	        ARRAY_of_UINT16 instance ids array. Each instance id is an id of 
    //              the resource instance that should be removed from the given resource.  
    //
    // ==Output==
    // Argument	    Type    Description
    // 0            UINT32  Temporary Cloud Connect Status of an attempt to emove resource 
    //                      instances. The final status will be signalled by 
    //                      RemoveResourceInstancesStatus.      
    //
    // ==Possible Cloud Connect Status values==
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
    // Emitted as a final status for RemoveResourceInstances asynchronous request.
    // This signal notifies an application that the process of removing resource 
    // instances is finished.  
    //
    // Argument	    Type	Description
    // 0	        UINT32	Final Cloud Connect Status of the RemoveResourceInstances.
    //
    // ==Possible Cloud Connect Status values==
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
    // ==Possible Cloud Connect Status values==
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
    // ==Possible Cloud Connect Status values==
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
    tr_debug("%s", __PRETTY_FUNCTION__);
    return ctx_.incoming_bus_message_callback_(m, userdata, ret_error);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////// API /////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// the userdata will be transferred as part of vtable object add
int DBusService_init(IncomingDataCallback callback)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    memset(&ctx_, 0, sizeof(ctx_));
    ctx_.incoming_bus_message_callback_ = callback;
    return 0;
}

int DBusService_deinit()
{   
    tr_debug("%s", __PRETTY_FUNCTION__);
    memset(&ctx_, 0, sizeof(ctx_));
    return 0;
}

const sd_bus_vtable* DBusService_get_service_vtable()
{
    return cloud_connect_service_vtable;
}

