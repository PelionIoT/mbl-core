/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DBusAdapterLowLevel_h_
#define _DBusAdapterLowLevel_h_

#include <inttypes.h>

#ifdef __cplusplus
extern "C"
{
#endif

// FIXME : remove later
#define tr_info(a, b)
#define tr_debug(a, b)
#define tr_error(a, b)

    typedef struct DBusAdapterCallbacks_
    {
        // Events coming from the D-Bus service
        int (*register_resources_async_callback)(const uintptr_t, const char *);
        int (*deregister_resources_async_callback)(const uintptr_t, const char *);

        // Other events
        int (*received_message_on_mailbox_callback)(const int, void*);
    } DBusAdapterCallbacks;

    int DBusAdapterLowLevel_init(const DBusAdapterCallbacks *adapter_callbacks);
    int DBusAdapterLowLevel_deinit();
    int DBusAdapterLowLevel_event_loop_run();
    int DBusAdapterLowLevel_event_loop_request_stop(int exit_code);
    int DBusAdapterLowLevel_event_loop_add_io(int fd, void *user_data);

#ifdef __cplusplus
} //extern "C" {
#endif

#endif // _DBusAdapterLowLevel_h_