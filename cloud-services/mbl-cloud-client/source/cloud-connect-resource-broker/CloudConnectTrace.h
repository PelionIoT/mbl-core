/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// TODO - decide if we want to keep this file before going back to master

/*
pre-processor macros which Add function name line number on top of mbed-trace/mbed_trace.h
There are additional macros to simplify logs + help debugging
*/

#ifndef _CloudConnectTrace_h_
#define _CloudConnectTrace_h_

#include "mbed-trace/mbed_trace.h"

// define _GNU_SOURCE in order to get the other safe version for basename, which doesn't modify 
// the __FILE__ string literal - this is to avoid compiler & static checkers warnings
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 
#endif    
#include <string.h>

#define TR_DEBUG(fmtstr, ...) \
    tr_debug("[%s:%s:%d]> " fmtstr, basename(__FILE__), __func__, __LINE__, ##__VA_ARGS__)
#define TR_INFO(fmtstr, ...) \
    tr_info("[%s:%s:%d]> " fmtstr, basename(__FILE__), __func__, __LINE__, ##__VA_ARGS__)
#define TR_WARN(fmtstr, ...) \
    tr_warn("[%s:%s:%d]> " fmtstr, basename(__FILE__), __func__, __LINE__, ##__VA_ARGS__)
#define TR_ERR(fmtstr, ...) \
    tr_err("[%s:%s:%d]> " fmtstr, basename(__FILE__), __func__, __LINE__, ##__VA_ARGS__)

/**
 * @brief Prints failed method name and errno in the constant format. 
 * @param failed_method_name failed method name(string).
 * @param errno_num errno number which should be logged.
 * @param fmtstr printf like format.
 * @param ... arguments
 */
#define TR_ERRNO_F(failed_method_name, errno_num, fmtstr, ...) \
    TR_ERR("%s failed with errno=%d (%s) " fmtstr, \
        failed_method_name, \
        errno_num, \
        strerror(errno_num), \
        ##__VA_ARGS__)


/**
 * @brief Prints failed method name and errno in the constant format. 
 * @param failed_method_name failed method name(string).
 * @param errno_num errno number which should be logged.
 */
#define TR_ERRNO(failed_method_name, errno_num) \
    TR_ERR("%s failed with errno=%d (%s)", failed_method_name, errno_num, strerror(errno_num))


// Use this macro to temporarily print debug points (usually on-target-debugging)
#define TR_DEBUG_POINT          TR_DEBUG("DBG_POINT")

// Use those 2 macros when entering and exiting functions 
#define TR_DEBUG_ENTER         TR_DEBUG("Enter")
#define TR_DEBUG_EXIT          TR_DEBUG("Exit")

#endif //_CloudConnectTrace_h_
