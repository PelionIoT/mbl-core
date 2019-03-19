/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "DBusAdapterCommon.h"
#include "CloudConnectTrace.h"

#include <sstream>

#define TRACE_GROUP "ccrb-dbus"

namespace mbl
{

int log_and_set_sd_bus_error_f(
    int err_num, sd_bus_error* ret_error, const char* func, int line, const std::stringstream& msg)
{
    if (0 < sd_bus_error_is_set(ret_error)) {
        // avoid overwrite of the error that was already set
        return sd_bus_error_get_errno(ret_error);
    }

    tr_err("%s::%d> %s", func, line, msg.str().c_str());
    return sd_bus_error_set_errnof(ret_error, err_num, "%s", msg.str().c_str());
}

/**
 * @brief Prints short error information to logs,
 * fills sd bus error structure and returns negative error.
 * If ret_error was already filled, returns an error that was set.
 *
 * @param err_num errno
 * @param ret_error sd-bus error description place-holder that is filled in the
 *                  case of an error.
 * @param func function name to put in logs
 * @param line line in file to put in logs
 * @param method_name failed method name.
 * @return int negative integer value that equals to -err_num if ret_error was not
 *             previously filled, or existing error from ret_error if the structure
 *             was already filled.
 */
int log_and_set_sd_bus_error(
    int err_num, sd_bus_error* ret_error, const char* func, int line, const char* method_name)
{

    if (0 < sd_bus_error_is_set(ret_error)) {
        // avoid overwrite of the error that was already set
        return sd_bus_error_get_errno(ret_error);
    }

    tr_err(
        "%s::%d> %s failed errno = %s (%d)", func, line, method_name, strerror(err_num), err_num);
    return sd_bus_error_set_errno(ret_error, err_num);
}

} // namespace mbl
