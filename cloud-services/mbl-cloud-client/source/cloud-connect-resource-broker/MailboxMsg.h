/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MailboxMsg_h_
#define _MailboxMsg_h_

#include "DBusAdapter.h"

#include <sstream>

namespace mbl {

struct MailboxMsg_Raw {
    char bytes[100];
};

struct MailboxMsg_Exit 
{   
    // THe reason for the exit, MblError::None if stop is done in normal way
    MblError     stop_status;
};

class MailboxMsg
{   
//Mailbox class is strongly coupled with MailboxMsg
friend class Mailbox;
public:        
    static const int MSG_PROTECTION_FIELD  = 0xFF128593;
    
    /**
     * @brief Construct a new Mailbox Msg object - 
     * Used ny MailBox to construct a new message
     * 
     * @param type - the message type
     * @param payload - the actual payload
     * @param payload_len - size of payload in bytes
     */
    template <typename T>
    MailboxMsg(T & data, size_t payload_len)
        : 
        serializer_(std::stringstream::in  |  std::stringstream::out | std::stringstream::binary),
        data_len_(payload_len),
        sequence_num_(get_next_sequence_num()),
        protection_field_(MSG_PROTECTION_FIELD)
    {
        mbed_tracef(TRACE_LEVEL_DEBUG, "ccrb-mailbox", "Enter");

        assert(payload_len <= sizeof(T));
        pack_data<T>(data);
    }

    MailboxMsg& operator = (MailboxMsg & src_msg) 
    { 
        data_len_ = src_msg.data_len_;
        data_type_name = src_msg.data_type_name;
        protection_field_ = src_msg.protection_field_;
        sequence_num_ = src_msg.sequence_num_;
        
        serializer_ << src_msg.serializer_.str();       
        assert(serializer_.tellp() > 0);
        return *this; 
    } 

    MailboxMsg(const mbl::MailboxMsg &src_msg)
    {
        data_len_ = src_msg.data_len_;
        data_type_name = src_msg.data_type_name;
        protection_field_ = src_msg.protection_field_;
        sequence_num_ = src_msg.sequence_num_;

        serializer_ << src_msg.serializer_.str();    
    }    

    template<typename T>
    void unpack_data(T & data)
    {
        mbed_tracef(TRACE_LEVEL_DEBUG, "ccrb-mailbox", "Enter");

        static_assert(std::is_trivial<T>::value && std::is_standard_layout<T>::value,
            "None POD types are not supported with this function!");

        decltype(sizeof(T)) size = sizeof(T);
        serializer_.read(reinterpret_cast<char*>(&data), size);
        assert(serializer_.tellp() > 0);
    }


    // Getters
    inline std::string &    get_data_type_name() { return data_type_name; }
    inline size_t           get_data_len() { return data_len_; }
    inline uint64_t         get_sequence_num() { return sequence_num_; }

private:         

    /**
     * @brief Construct a new Mailbox Msg empty object
     * To be used by mailbox only
     * 
     */
     MailboxMsg() 
        :
        data_len_(0),
        sequence_num_(0),
        protection_field_(MSG_PROTECTION_FIELD)
    {
        mbed_tracef(TRACE_LEVEL_DEBUG, "ccrb-mailbox", "Enter");
    }


    template<typename T>
    void pack_data(T const& data)
    {
        mbed_tracef(TRACE_LEVEL_DEBUG, "ccrb-mailbox", "Enter");

        // this only works on built in data types (PODs)
        static_assert(std::is_trivial<T>::value && std::is_standard_layout<T>::value,
            "None POD types are not supported with this function!");

        serializer_.write(reinterpret_cast<char const*>(&data), sizeof(T));        
        data_type_name.assign(typeid(T).name());
        assert(serializer_.tellp() == sizeof(T));      
    }

    // The actual data holder (data is serialized into a standard library string buffer)
    std::stringstream serializer_;

    // size in bytes for the payload - given by user and can't be more then the maximum 
    // message allowed by type
    size_t      data_len_;

    // message sequence number
    uint64_t    sequence_num_;

    //protection field to assert for integrity
    long    protection_field_ = 0;

    // information about the type stored
    std::string data_type_name;

    // used to auto-assign message sequence number
    // we can't initialize static members in header, so I workaround this using static member 
    // function, while the static member is define inside function
    static uint64_t get_next_sequence_num() {
        static uint64_t sequence_num_counter_ = 1;
        return sequence_num_counter_++;
    };
};

}//namespace mbl

#endif // _MailboxMsg_h_
  