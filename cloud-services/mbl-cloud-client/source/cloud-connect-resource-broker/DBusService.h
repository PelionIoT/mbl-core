/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef _DBusService_h_
#define _DBusService_h_

#include <inttypes.h>

#include <systemd/sd-bus.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef int (*IncomingDataCallback)(sd_bus_message*, void*, sd_bus_error*);

void                    DBusService_init(IncomingDataCallback incoming_data_callback);
void                    DBusService_deinit();
const sd_bus_vtable*    DBusService_get_service_vtable();

#ifdef __cplusplus
} //extern "C" {
#endif

#endif //_DBusService_h_