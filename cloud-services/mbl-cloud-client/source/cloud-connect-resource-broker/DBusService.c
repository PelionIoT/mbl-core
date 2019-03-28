/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "DBusService.h"
#include "DBusCloudConnectNames.h"

#include <assert.h>

#define TRACE_GROUP "ccrb-dbus"

/**
 * @brief The module context type carries the callback to be invoked when a bus message arrived
 * I use a single callback to the C++ code in order to reduce the amount of code. The C++ code will
 * multiplex the callback into message-specific processing function
 *
 */
typedef struct DBusServiceContext_
{
    IncomingBusMessageCallback incoming_bus_message_callback_;
} DBusServiceContext;

static DBusServiceContext ctx_ = {0};

/**
 * @brief This is the actual callback attached on the cloud_connect_service_vtable vtable
 * per each interface member. It is of type sd_bus_message_handler_t. It straight calls the
 * supplied DBusServiceContext::incoming_bus_message_callback_ C++ static callback
 *
 * @param m - handle to the message received
 * @param userdata - userdata supplied while calling sd_bus_attach_event() -
 * always 'this' (DBusAdapter_Impl)
 * @param ret_error - The sd_bus_error structure carries information about a D-Bus error
 * @return int -  0 on success, On failure, return a negative Linux errno-style error code
 */
static int
incoming_bus_message_callback(sd_bus_message* m, void* userdata, sd_bus_error* ret_error);

/**
 * @brief This is the sd_bus_vtable which defined the interface DBUS_CLOUD_CONNECT_INTERFACE_NAME
  * under object DBUS_CLOUD_CONNECT_OBJECT_PATH. It is attached to the sd-bus connection object
  * (which acquires the known name DBUS_CLOUD_SERVICE_NAME)  by the DBusAdapter_Impl by calling
  * sd_bus_add_object_vtable(). It contains all method calls, signals and properties to be
  * published.
  * It may contains also hidden methods.
 */
const sd_bus_vtable cloud_connect_service_vtable[] = {
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

    // com.mbed.Pelion1.Connect.RegisterResources
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
    SD_BUS_METHOD(DBUS_CC_REGISTER_RESOURCES_METHOD_NAME,
                  "s",
                  "s",
                  incoming_bus_message_callback,
                  SD_BUS_VTABLE_UNPRIVILEGED),

    // TODO: This signal is disabled for now
    // com.mbed.Pelion1.Connect.RegisterResourcesStatus
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
    // SD_BUS_SIGNAL("RegisterResourcesStatus", "u", 0),

    // TODO: This method call is disabled for now - upper layer does not support it as expected.
    // com.mbed.Pelion1.Connect.DeregisterResources
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
    // FIXME - This method should be uncommented after we deregistration is supported on higher
    // layers.
    // SD_BUS_METHOD(
    //    "DeregisterResources", "s", "u", incoming_bus_message_callback,
    //    SD_BUS_VTABLE_UNPRIVILEGED),

    // com.mbed.Pelion1.Connect.DeregisterResourcesStatus
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
    // SD_BUS_SIGNAL("DeregisterResourcesStatus", "u", 0),

    // TODO: This method call is disabled for now
    // com.mbed.Pelion1.Connect.AddResourceInstances
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
    // SD_BUS_METHOD("AddResourceInstances",
    //               "ssaq",
    //               "u",
    //               incoming_bus_message_callback,
    //               SD_BUS_VTABLE_UNPRIVILEGED),

    // TODO: This signal is disabled for now
    // com.mbed.Pelion1.Connect.AddResourceInstancesStatus
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
    // SD_BUS_SIGNAL("AddResourceInstancesStatus", "u", 0),

    // TODO: This method call is disabled for now
    // com.mbed.Pelion1.Connect.RemoveResourceInstances
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
    // SD_BUS_METHOD("RemoveResourceInstances",
    //               "ssaq",
    //               "u",
    //               incoming_bus_message_callback,
    //               SD_BUS_VTABLE_UNPRIVILEGED),

    // TODO: This signal is disabled for now
    // com.mbed.Pelion1.Connect.RemoveResourceInstancesStatus
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
    // SD_BUS_SIGNAL("RemoveResourceInstancesStatus", "u", 0),

    // com.mbed.Pelion1.Connect.SetResourcesValues
    //
    // As a Method :
    // ARRAY_of_UINT32 SetResourcesValues(STRING access_token,
    //                                    ARRAY_of_STRUCTS(STRING,UINT8,VARIANT)
    //                                    set_operation_input)
    //
    // Description :
    // Request to set resources values for multiple resources.
    //
    // ==Input==
    // Argument	    Type                             Description
    // 0	        STRING                           access-token
    // 1	        ARRAY_of_STRUCTS(STRING,VARIANT) array of structs that contains set
    // operation
    //                                               input. Each struct in the array
    //                                                     contains:
    //                                                     - path of the resource (STRING)
    //                                                     - resource value (VARIANT)
    //
    // ==Output==
    // Argument	    Type                             Description
    //                                               Empty reply
    //
    // Argument     Type                             Description
    // 0            STRING                           Error reply will be sent if one or more get
    //                                               resources values operations fail.
    //                                               An error could contain a description of maximum
    //                                               10
    //                                               resource paths and the correspondent errors
    //                                               types.
    //                                               In case of invalid access token error or other
    //                                               error
    //                                               related to all resources in the request, error
    //                                               reply
    //                                               will only include this error.
    //
    // ==Possible Cloud Connect Error values==
    // TBD
    SD_BUS_METHOD(DBUS_CC_SET_RESOURCES_VALUES_METHOD_NAME,
                  "sa(sv)",
                  "",
                  incoming_bus_message_callback,
                  SD_BUS_VTABLE_UNPRIVILEGED),

    // com.mbed.Pelion1.Connect.GetResourcesValues
    //
    // As a Method :
    // ARRAY_of_STRUCTS(UINT32,UINT8,VARIANT) GetResourcesValues(STRING access_token,
    //                                                           ARRAY_of_STRUCTS(STRING,UINT8)
    //                                                           get_operation_input)
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
    // Argument	    Type                            Description
    // 0	        ARRAY_of_STRUCTS(UINT8,VARIANT) array of structs that contains get
    // operation
    //                                              output for each entry in the input array.
    //                                              Each struct contains:
    //                                              - resource [i] data type (UINT8). Valid only if
    //                                              the status
    //                                                of the get operation is SUCCESS.
    //                                              - resource [i] value (VARIANT). Valid only if
    //                                              the status of the get operation is SUCCESS.
    //
    // ==Error Reply==
    // Argument     Type                             Description
    // 0            STRING                           Error reply will be sent if one or more get
    //                                               resources values operations fail.
    //                                               An error could contain a description of maximum
    //                                               10
    //                                               resource paths and the correspondent errors
    //                                               types.
    //                                               In case of invalid access token error or other
    //                                               error
    //                                               related to all resources in the request, error
    //                                               reply
    //                                               will only include this error.
    //
    // ==Possible Cloud Connect Error values==
    // TBD
    SD_BUS_METHOD(DBUS_CC_GET_RESOURCES_VALUES_METHOD_NAME,
                  "sa(sy)",
                  "a(yv)",
                  incoming_bus_message_callback,
                  SD_BUS_VTABLE_UNPRIVILEGED),

    SD_BUS_VTABLE_END};

static int incoming_bus_message_callback(sd_bus_message* m, void* userdata, sd_bus_error* ret_error)
{
    assert(m);
    assert(ret_error);
    return ctx_.incoming_bus_message_callback_(m, userdata, ret_error);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////// API /////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// the userdata will be transferred as part of vtable object add
void DBusService_init(IncomingBusMessageCallback callback)
{
    assert(callback);
    memset(&ctx_, 0, sizeof(ctx_));
    // set a static callback back to DBusAdapter_Impl object.
    // comment : std::bind is not possible currently
    ctx_.incoming_bus_message_callback_ = callback;
}

void DBusService_deinit()
{
    memset(&ctx_, 0, sizeof(ctx_));
}

const sd_bus_vtable* DBusService_get_service_vtable()
{
    return cloud_connect_service_vtable;
}
