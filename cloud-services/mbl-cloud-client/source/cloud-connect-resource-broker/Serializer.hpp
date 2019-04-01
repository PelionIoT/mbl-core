/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _Serializer_hpp_
#define _Serializer_hpp_

#include "CloudConnectTrace.h"
#include "MblError.h"

#include <sstream>

namespace mbl
{

/**
 * @brief Serialize a POD data into a given string buffer serializer
 * This function is used by MailboxMsg and Event classes
 * @param T - the type of data to serialize - must be POD
 * @param trace_group - used for logging (replace TRACE_GROUP)
 * @param data - the data template type to serialize. must be POD.
 * @param serializer - the string buffer to serialize data to
 * @return MblError - SystemCallFailed on failure, None on success
 */
template <typename T>
MblError pack_data(const char* trace_group, T const& data, std::stringstream& serializer)
{
    mbed_tracef(TRACE_LEVEL_DEBUG, trace_group, "Enter");

    // Static assert - make sure user will try to compile only built in data types (PODs)
    static_assert(std::is_trivial<T>::value && std::is_standard_layout<T>::value,
                  "None POD types are not supported with this function!");

    serializer.write(reinterpret_cast<char const*>(&data), sizeof(T));
    if (!serializer.good()) {
        mbed_tracef(TRACE_LEVEL_ERROR,
                    trace_group,
                    "serializer.write failed! failbit=%d"
                    " eofbit=%d badbit=%d - returning error %s",
                    serializer.fail(),
                    serializer.eof(),
                    serializer.bad(),
                    MblError_to_str(MblError::SystemCallFailed));
        return MblError::SystemCallFailed;
    }
    return MblError::None;
}

/**
 * @brief Deserialize a POD data from a given string buffer serializer
 * This function is used by MailboxMsg and Event classes
 * @param T - the type of data to serialize - must be POD
 * @param trace_group - used for logging (replace TRACE_GROUP)
 * @param serializer - the string buffer to serialize data to
 * @param expected_data_size - expected data size to be unpacked
 * @return std::pair<MblError, T> -  pair where the first element is the status. SystemCallFailed
 * for failure or None for success. The 2nd element is relevant only on success, and is the
 * serialized data
 */
template <typename T>
std::pair<MblError, T> unpack_data(const char* trace_group,
                                   std::stringstream& serializer,
                                   size_t expected_data_size)
{
    mbed_tracef(TRACE_LEVEL_DEBUG, trace_group, "Enter");

    // Static assert - make sure user will try to compile only built in data types (PODs)
    static_assert(std::is_trivial<T>::value && std::is_standard_layout<T>::value,
                  "None POD types are not supported with this function!");
    T data;
    decltype(sizeof(T)) size = sizeof(T);
    // Validate data size (sanity check)
    if(size != expected_data_size) {
        mbed_tracef(TRACE_LEVEL_ERROR,
                    trace_group,
                    "Unexpected data size %zu (expected %zu)",
                    size,
                    expected_data_size);
        return std::make_pair(MblError::SystemCallFailed, data);
    }
    serializer.read(reinterpret_cast<char*>(&data), size);
    if (!serializer.good()) {
        mbed_tracef(TRACE_LEVEL_ERROR,
                    trace_group,
                    "serializer.read failed! failbit=%d"
                    " eofbit=%d badbit=%d - returning error %s",
                    serializer.fail(),
                    serializer.eof(),
                    serializer.bad(),
                    MblError_to_str(MblError::SystemCallFailed));
        return std::make_pair(MblError::SystemCallFailed, data);
    }
    // After reading data, seek to start so in case of periodic event the data will be available:
    serializer.clear(); // clear the state of the stream
    serializer.seekg(0, std::ios::beg); // seek start
    return std::make_pair(MblError::None, data);
}

} // namespace mbl

#endif //_Serializer_hpp_