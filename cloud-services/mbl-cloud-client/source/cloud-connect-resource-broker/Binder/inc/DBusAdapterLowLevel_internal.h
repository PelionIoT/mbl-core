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

//TODO cnsider change name of this header to match cpp and the other one to end wit API, or just combine
#include "DBusAdapterLowLevel.h"

#ifdef __cplusplus
extern "C" {
#endif 

#define tr_info(a,b)
#define tr_debug(a,b)
#define tr_error(a,b)

//int incoming_bus_message_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);


typedef struct DBusAdapterLowLevelContext_
{   
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


//extern DBusAdapterLowLevelContext ctx_;

#ifdef __cplusplus
} // extern "C" {
#endif

#endif // _DBusAdapterLowLevel_internal_h_