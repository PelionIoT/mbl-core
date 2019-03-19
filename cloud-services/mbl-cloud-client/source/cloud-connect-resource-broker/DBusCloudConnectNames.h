/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DBusCloudConnectNames_h_
#define _DBusCloudConnectNames_h_

// sd-bus vtable object, implements the com.mbed.Pelion1.Connect interface
// Keep those definitions here for testing
#define DBUS_CLOUD_SERVICE_NAME "com.mbed.Pelion1"
#define DBUS_CLOUD_CONNECT_INTERFACE_NAME "com.mbed.Pelion1.Connect"
#define DBUS_CLOUD_CONNECT_OBJECT_PATH "/com/mbed/Pelion1/Connect"

// methods
#define DBUS_CC_REGISTER_RESOURCES_METHOD_NAME "RegisterResources"
#define DBUS_CC_DEREGISTER_RESOURCES_METHOD_NAME "DeregisterResources"
#define DBUS_CC_ADD_RESOURCE_INSTANCES_METHOD_NAME "AddResourceInstances"
#define DBUS_CC_REMOVE_RESOURCE_INSTANCES_METHOD_NAME "RemoveResourceInstances"
#define DBUS_CC_GET_RESOURCES_VALUES_METHOD_NAME "GetResourcesValues"
#define DBUS_CC_SET_RESOURCES_VALUES_METHOD_NAME "SetResourcesValues"

// signals
#define DBUS_CC_REGISTER_RESOURCES_STATUS_SIGNAL_NAME "RegisterResourcesStatus"
#define DBUS_CC_DEREGISTER_RESOURCES_STATUS_SIGNAL_NAME "DeregisterResourcesStatus"
#define DBUS_CC_ADD_RESOURCE_INSTANCES_STATUS_SIGNAL_NAME "AddResourceInstancesStatus"
#define DBUS_CC_REMOVE_RESOURCE_INSTANCES_STATUS_SIGNAL_NAME "RemoveResourceInstancesStatus"

#endif //_DBusCloudConnectNames_h_
