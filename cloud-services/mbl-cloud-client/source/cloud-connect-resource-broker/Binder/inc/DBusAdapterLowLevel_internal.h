/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DBusAdapterLowLevel_internal_h_
#define _DBusAdapterLowLevel_internal_h_

#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>
#include <pthread.h>

#include "DBusAdapterLowLevel.h"

#ifdef __cplusplus
extern "C" {
#endif 

#define tr_info(a,b)
#define tr_debug(a,b)
#define tr_error(a,b)


// sd-bus vtable object, implements the com.mbed.Cloud.Connect1 interface
#define DBUS_CLOUD_SERVICE_NAME                 "com.mbed.Cloud"
#define DBUS_CLOUD_CONNECT_INTERFACE_NAME       "com.mbed.Cloud.Connect1"
#define DBUS_CLOUD_CONNECT_OBJECT_PATH          "/com/mbed/Cloud/Connect1"

typedef struct DBusAdapterLowLevelContext_
{   
    pthread_t               master_thread_id;       // The "master thread" is the one which initializes this module
                                                    // This should be the CCRB thread
    // D-Bus
    sd_event                *event_loop_handle;
    sd_bus                  *connection_handle;
    sd_bus_slot             *connection_slot;         // TODO : needed?
    const char              *unique_name;
    char                    *service_name;    
    
    // event loop
    DBusAdapterCallbacks    adapter_callbacks;
    void                    *adapter_callbacks_userdata;
    sd_event_source         *event_source_pipe;    
}DBusAdapterLowLevelContext;

#ifdef __cplusplus
} // extern "C" {
#endif

#endif // _DBusAdapterLowLevel_internal_h_