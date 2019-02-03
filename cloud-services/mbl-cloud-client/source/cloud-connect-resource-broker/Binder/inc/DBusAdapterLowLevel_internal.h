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
{   sd_event                *event_loop_handle;
    sd_bus                  *connection_handle;
    sd_bus_slot             *connection_slot;         // TODO : needed?
    const char              *unique_name;
    char                    *service_name;
    DBusAdapterCallbacks    adapter_callbacks;
    pthread_t               ccrb_thread_id;

    //event sources
    sd_event_source *event_source_pipe;
}DBusAdapterLowLevelContext;

#ifdef __cplusplus
} // extern "C" {
#endif

#endif // _DBusAdapterLowLevel_internal_h_