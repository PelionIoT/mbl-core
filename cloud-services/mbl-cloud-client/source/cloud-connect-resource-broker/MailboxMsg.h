/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MailboxMsg_h_
#define _MailboxMsg_h_

#include "DBusAdapter.h"

namespace mbl{

#define DBUS_MAILBOX_RAW_MSG_MAX_PAYLOAD_SIZE       100
#define DBUS_MAILBOX_MSG_PROTECTION_FIELD           0xFF128593




//TODO :  add 2 static compile time casts to check that 
//DBUS_MAX_MSG_RAW_PAYLOAD_SIZE<DBUS_MAX_MSG_SIZE and that max size of MailboxMsg
// is less than  MAX_MSG_SIZE



// TODO : this struct has 2 members that should not be changed externaly
// transforming it ointo class requiers extra work - more getters, copy ctor and =operator
// I do not have time for this now
struct MailboxMsg
{   
    typedef struct Msg_raw_ 
    {
        char    bytes[DBUS_MAILBOX_RAW_MSG_MAX_PAYLOAD_SIZE];
    }Msg_raw_;

    typedef struct Msg_exit_ 
    {
        MblError     stop_status;
    }Msg_exit_;

    // Keep this enumerator
    enum class MsgType
    {
        RAW_DATA = 1,   // 
        EXIT = 2        // exit
    };

    typedef union MsgPayload_
    {
        Msg_raw_  raw;
        Msg_exit_ exit;
    }MsgPayload_; 


    // TODO : move to file - consider ctor per message type or inheritance
    //A message is valid if protection field is valid and payload is non 0
    //bool        is_valid_msg() { return (sequence_num == PROTECTION_FIELD) && (payload_len > 0); } 
    //uint64_t    get_sequence_num() { return sequence_num; }   

    size_t      payload_len_;                 // size in bytes of payload field
    MsgType     type_;  
    MsgPayload_ payload_;

    //private:    
    uint64_t      _sequence_num;
    uint64_t      _protection_field;   //CONSIDER?
};

}//namespace mbl

#endif // _MailboxMsg_h_