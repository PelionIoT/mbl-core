/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef _DBusService_h_
#define _DBusService_h_

#include <systemd/sd-bus.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
Why we use a C module?
Multiple macros in sd-bus-vtable.h does not support natively C++ compilation
The compiler warns or generate errors. I prefered not to patch the systemd library so the 
sd_bus_vtable part (our D-Bus service declaration) is written in compiled in C.
*/

/**
 * @brief This callback type is used in order to set the callback to be called from the 
 * DBusService.c
 * module into the C++ DBusAdapter_Impl object.
 * 
 */
typedef int (*IncomingBusMessageCallback)(sd_bus_message*, void*, sd_bus_error*);

/**
 * @brief  set a static C+ callback into the DBusService module, to be called when a D-Bus message
 * arrives.
 * 
 * @param incoming_data_callback 
 */
void                    DBusService_init(IncomingBusMessageCallback incoming_data_callback);

/**
 * @brief reset the callback (can be removed but I used it for now as a place holder in case we 
 * preform advanced dynamic interface operations in the future.
 * 
 */
void                    DBusService_deinit();

/**
 * @brief 
 * Clled when attaching the vtable to the bus connection object, in order to publish it on the bus.
 * @return const sd_bus_vtable* 
 */
const sd_bus_vtable*    DBusService_get_service_vtable();

#ifdef __cplusplus
} //extern "C" {
#endif

#endif //_DBusService_h_