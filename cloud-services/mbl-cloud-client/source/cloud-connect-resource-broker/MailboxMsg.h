/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MailboxMsg_h_
#define _MailboxMsg_h_

#include "DBusAdapter.h"
#include "Serializer.hpp"

#include <sstream>

namespace mbl
{

class MailboxMsg
{
    // Mailbox class is strongly coupled with MailboxMsg
    friend class Mailbox;

public:
    static const long int MSG_PROTECTION_FIELD = 0xFF128593L;

    /**
     * @brief Construct a new Mailbox Msg object -
     * Used ny MailBox to construct a new message
     *
     * @param data - the actual payload
     * @param data_len - size of payload in bytes
     */
    template <typename T>
    MailboxMsg(T& data, size_t data_len)
        : data_len_(data_len),
          sequence_num_(get_next_sequence_num()),
          protection_field_(MSG_PROTECTION_FIELD)
    {
        mbed_tracef(TRACE_LEVEL_DEBUG, "ccrb-mailbox", "Enter");

        assert(data_len <= sizeof(T));
        pack_data<T>(data);
        data_type_name_.assign(typeid(T).name());
    }

    /**
     * @brief assignment operator implementation
     *
     * @param src_msg - the right value message to assign from
     * @return MailboxMsg& - the message to assign to
     */
    MailboxMsg& operator=(MailboxMsg& src_msg)
    {
        data_len_ = src_msg.data_len_;
        data_type_name_ = src_msg.data_type_name_;
        protection_field_ = src_msg.protection_field_;
        sequence_num_ = src_msg.sequence_num_;

        serializer_ << src_msg.serializer_.str();
        assert(serializer_.tellp() > 0);
        return *this;
    }

    /**
     * @brief Copy ctor
     *
     * @param src_msg - message to copy from
     */
    MailboxMsg(const mbl::MailboxMsg& src_msg)
    {
        data_len_ = src_msg.data_len_;
        data_type_name_ = src_msg.data_type_name_;
        protection_field_ = src_msg.protection_field_;
        sequence_num_ = src_msg.sequence_num_;

        serializer_ << src_msg.serializer_.str();
    }

    /**
     * @brief Templated deserialization function. Fill a reference of type T by reading cytes from
     * serializer_.
     *
     * @param expected_msg_size - expected data size to be unpacked
     * @return std::pair<MblError, T> - a pair with :
     * First element - the result of the operation, SystemCallFailed for failure.
     * Second element - relevant only on success - T unpacked by NRVO
     * (Named Return Value Optimization)
     */
    template <typename T>
    std::pair<MblError, T> unpack_data(size_t expected_msg_size)
    {
        mbed_tracef(TRACE_LEVEL_DEBUG, "ccrb-mailbox", "Enter");

        // Static assert - make sure user will try to compile only built in data types (PODs)
        static_assert(std::is_trivial<T>::value && std::is_standard_layout<T>::value,
                      "None POD types are not supported with this function!");

        return mbl::unpack_data<T>("ccrb-mailbox", serializer_, expected_msg_size);
    }

    // Getters
    inline std::string& get_data_type_name() { return data_type_name_; }
    inline size_t get_data_len() { return data_len_; }
    inline uint64_t get_sequence_num() { return sequence_num_; }

private:
    /**
     * @brief Construct a new Mailbox Msg empty object
     * To be used by mailbox only
     *
     */
    MailboxMsg() : data_len_(0), sequence_num_(0), protection_field_(MSG_PROTECTION_FIELD)
    {
        mbed_tracef(TRACE_LEVEL_DEBUG, "ccrb-mailbox", "Enter");
    }

    /**
  * @brief  Serialize data of type T into serializer_ (standard library  string buffer)
  *
  * @tparam T - The type to pack (serialize) - must be POD type.
  * @param data - data of type T to serialize.
  * @return MblError - SystemCallFailed on failure, else None;
  */
    template <typename T>
    MblError pack_data(T const& data)
    {
        mbed_tracef(TRACE_LEVEL_DEBUG, "ccrb-mailbox", "Enter");

        // Static assert - make sure user will try to compile only built in data types (PODs)
        static_assert(std::is_trivial<T>::value && std::is_standard_layout<T>::value,
                      "None POD types are not supported with this function!");

        data_type_name_.assign(typeid(T).name());
        return mbl::pack_data<T>("ccrb-mailbox", data, serializer_);
    }

    // The actual data holder (data is serialized into a standard library string buffer)
    std::stringstream serializer_;

    // size in bytes for the payload - given by user and can't be more then the maximum
    // message allowed by type
    size_t data_len_;

    // message sequence number
    uint64_t sequence_num_;

    // protection field to assert for integrity
    long int protection_field_ = 0;

    // information about the type stored
    std::string data_type_name_;

    // used to auto-assign message sequence number
    // we can't initialize static members in header, so I workaround this using static member
    // function, while the static member is define inside function
    static uint64_t get_next_sequence_num()
    {
        static uint64_t sequence_num_counter_ = 1;
        return sequence_num_counter_++;
    };
};

} // namespace mbl

#endif // _MailboxMsg_h_
