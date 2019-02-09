/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TestInfraCommon_h_
#define _TestInfraCommon_h_


#define TI_SLEEP_MS(time_to_sleep_im_ms) usleep(1000 * time_to_sleep_im_ms)


// In some tests, for simplicity, threads are not synchronized
// In the real code they are -> using the event loop or other means. That's why the actual mailbox code
// does not implement retries on timeout polling failures.
// Since they are synchronized, read should always succeed.
// That's why I wait up to 100 ms
#define TI_DBUS_MAILBOX_MAX_WAIT_TIME_MS 100

#endif //#ifndef _TestInfraCommon_h_