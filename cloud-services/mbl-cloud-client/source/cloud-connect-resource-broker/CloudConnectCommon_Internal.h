/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CloudConnectCommon_Internal_h_
#define _CloudConnectCommon_Internal_h_

#include "mbed-trace/mbed_trace.h"

// Simple stringify
#define stringify(s) #s

// If you want to stringify the result of expansion of a macro argument, you have to use two 
// levels of macros. (use xstringify())
#define xstringify(s) stringify(s)

// to mark unused variable for the preprocessor
#define UNUSED(x) (void)(x)

// TODO - remove OR approve before re-basing to back to main mbl-core branch
// adds line and function name as part of the Mbed trace logger.
// !!! keep full width even though it passes 100 chars !!!
#if MBED_TRACE_MAX_LEVEL >= TRACE_LEVEL_DEBUG
#undef tr_debug
#define tr_debug(...)  mbed_tracef(TRACE_LEVEL_DEBUG,   TRACE_GROUP, __PRETTY_FUNCTION__, "::", __LINE__, #__VA_ARGS__) 
#endif

#if MBED_TRACE_MAX_LEVEL >= TRACE_LEVEL_INFO
#undef tr_info
#define tr_info(...)  mbed_tracef(TRACE_LEVEL_INFO,   TRACE_GROUP, __PRETTY_FUNCTION__, "::", __LINE__, #__VA_ARGS__) 
#endif

#if MBED_TRACE_MAX_LEVEL >= TRACE_LEVEL_WARN
#undef tr_warning
#undef tr_warn
#define tr_warning(...)  mbed_tracef(TRACE_LEVEL_WARN,   TRACE_GROUP, __PRETTY_FUNCTION__, "::", __LINE__, ":", #__VA_ARGS__) 
#define tr_warn(...)  mbed_tracef(TRACE_LEVEL_WARN,   TRACE_GROUP, __PRETTY_FUNCTION__, "::", __LINE__, ":", #__VA_ARGS__)
#endif

#if MBED_TRACE_MAX_LEVEL >= TRACE_LEVEL_ERROR
#undef tr_error
#undef tr_err
#define tr_error(...)  mbed_tracef(TRACE_LEVEL_ERROR,   TRACE_GROUP, __PRETTY_FUNCTION__, "::", __LINE__, ":", #__VA_ARGS__) 
#define tr_err(...)  mbed_tracef(TRACE_LEVEL_ERROR,   TRACE_GROUP, __PRETTY_FUNCTION__, "::", __LINE__, ":", #__VA_ARGS__)
#endif

#endif //_CloudConnectCommon_Internal_h_