/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// TODO - decide if we want to keep this file before merginggoing back to master

/*
pre-processor macros which Add function name line number on top of mbed-trace/mbed_trace.h
There are additional macros to simplify logs + help debugging
*/

#ifndef _CloudConnectTrace_h_
#define _CloudConnectTrace_h_

#include "mbed-trace/mbed_trace.h"

#include <string.h>

// truncate __FILE__ to a file name without the full path
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define TR_DEBUG(fmtstr, ...) \
    tr_debug("[%s:%s:%d]> " fmtstr,  __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
#define TR_INFO(fmtstr, ...) \
    tr_info("[%s:%s:%d]> " fmtstr,  __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
#define TR_WARN(fmtstr, ...) \
    tr_warn("[%s:%s:%d]> " fmtstr,  __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
#define TR_ERR(fmtstr, ...) \
    tr_err("[%s:%s:%d]> " fmtstr,  __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)

//use this macro to temporarily print debug points (usually on-target-debugging)
#define TR_DEBUG_POINT          TR_DEBUG("DBG_POINT")

#endif //_CloudConnectTrace_h_
