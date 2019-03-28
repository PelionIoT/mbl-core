/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DBusAdapterCommon_h_
#define _DBusAdapterCommon_h_

#include <functional>
#include <string>

#include <systemd/sd-bus.h>

/**
 * @brief Wrapper over log_and_set_sd_bus_error_f.
 *
 * @param err_num errno
 * @param ret_error sd-bus error description place-holder that is filled in the
 *                  case of an error.
 * @param stringstream object that should be printed
 */
#define LOG_AND_SET_SD_BUS_ERROR_F(err_num, ret_error, stringstream)                               \
    log_and_set_sd_bus_error_f(err_num, ret_error, __func__, __LINE__, stringstream)

/**
 * @brief Wrapper over log_and_set_sd_bus_error.
 *
 * @param err_num errno
 * @param ret_error sd-bus error description place-holder that is filled in the
 *                  case of an error.
 * @param method_name failed method name.
 */
#define LOG_AND_SET_SD_BUS_ERROR(err_num, ret_error, method_name)                                  \
    log_and_set_sd_bus_error(err_num, ret_error, __func__, __LINE__, method_name)

namespace mbl
{

/**
 * @brief Prints error information to logs according to provided format,
 * fills sd bus error structure and returns negative error.
 * If ret_error was already filled, returns an error that was set.
 *
 * @param err_num errno
 * @param ret_error sd-bus error description place-holder that is filled in the
 *                  case of an error.
 * @param func function name to put in logs
 * @param line line in file to put in logs
 * @param msg object that should be printed
 * @return int negative integer value that equals to -err_num if ret_error was not
 *             previously filled, or existing error from ret_error if the structure
 *             was already filled.
 */
int log_and_set_sd_bus_error_f(
    int err_num, sd_bus_error* ret_error, const char* func, int line, const std::stringstream& msg);

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
    int err_num, sd_bus_error* ret_error, const char* func, int line, const char* method_name);

/**
 * @brief sd-bus objects resource manager class.
 * s-dbus objects requires resource management, as described on this link:
 * https://www.freedesktop.org/software/systemd/man/sd_bus_new.html#
 * This class implements basic resource management logic.
 *
 * @tparam T sd-bus object (like sd_bus_message)
 */
template <class T>
class sd_bus_object_cleaner
{
public:
    /**
     * @brief Construct a new sd objects cleaner for the provided sd-bus specific object type.
     *
     * @param object_to_clean address of the sd-bus object.
     * @param func cleaning function, that gets address of the sd-bus object.
     *        Typical cleaning functions described on this link:
     *        https://www.freedesktop.org/software/systemd/man/sd_bus_new.html#
     *        This clean function will be called in destructor of the object.
     */
    sd_bus_object_cleaner(T* object_to_clean, std::function<void(T*)> func)
        : object_(object_to_clean), clean_func_(func)
    {
    }

    ~sd_bus_object_cleaner() { clean_func_(object_); }

private:
    T* object_;
    std::function<void(T*)> clean_func_;
};

} // namespace mbl

#endif //_DBusAdapterCommon_h_
