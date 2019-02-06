/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef _DBusAdapterService_h_
#define _DBusAdapterService_h_

#include <inttypes.h>

#include <systemd/sd-bus.h>

#ifdef __cplusplus
extern "C"
{
#endif

// FIXME : remove later
#define tr_info(a, b)
#define tr_debug(a, b)
#define tr_error(a, b)


typedef int (*IncomingDataCallback)(sd_bus_message*, void*, sd_bus_error*);


int                     DBusAdapterService_init(IncomingDataCallback incoming_data_callback);
int                     DBusAdapterService_deinit();
const sd_bus_vtable*    DBusAdapterService_get_service_vtable();

#ifdef __cplusplus
} //extern "C" {
#endif

#endif //_DBusAdapterService_h_