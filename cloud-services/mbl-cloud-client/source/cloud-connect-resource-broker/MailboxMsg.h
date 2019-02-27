/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MailboxMsg_h_
#define _MailboxMsg_h_

#include "DBusAdapter.h"

#include <vector>

namespace mbl{

class MailboxMsg
{   
//Mailbox class is strongly coupled with MailboxMsg
friend class Mailbox;
public:        
    static const int MSG_PROTECTION_FIELD  = 0xFF128593;

    //TODO - get rid of UKNOWN message type by design. do not allow empty messages to be 
    // constructed.
    /**
     * @brief The message type
     * 
     */
    enum class MsgType {
        UNKNOWN = 0,        // temporary type assigned for empty message
        RAW_DATA = 1,       // a generic message type (bytes)
        EXIT = 2            // sent by an external thread to request CCRB thread to exit 
                            // from event loop
    };

    // TODO - IOTMBL-1684 - consider improving using template / polymorphic style
    /**
     * @brief - Holds all possible messages as a union, and the length. The actual message type is 
     * given by 'Mailbox::type_'. Each message is limited by size - developer should extend message 
     * using the message macros.
    */
    typedef union MsgPayload_
    {
        struct MsgRaw {
            static constexpr int MAX_SIZE = 100;
            char bytes[MAX_SIZE];
        }raw_;
        
        struct MsgExit 
        {   
            // THe reason for the exit, MblError::None if stop is done in normal way
            MblError     stop_status;
        }exit_;
    }MsgPayload; 


    /**
     * @brief Construct a new Mailbox Msg object - 
     * Used ny MailBox to construct a new message
     * 
     * @param type - the message type
     * @param payload - the actual payload
     * @param payload_len - size of payload in bytes
     */
    MailboxMsg(MsgType type, MsgPayload &payload, size_t payload_len); 

    /**
     * @brief stringify function for the MsgType
     * 
     * @return const char* 
     */
    const char* MsgType_to_str();

    // Getters
    inline MsgType         get_type() { return type_; }
    inline MsgPayload&     get_payload() { return payload_; }
    inline size_t          get_payload_len() { return payload_len_; }
    inline uint64_t        get_sequence_num() { return sequence_num_; }

private:         

    /**
     * @brief Construct a new Mailbox Msg empty object
     * To be used by mailbox only
     * 
     */
    MailboxMsg();  

    // The actual data holder. use the type to determine format
    MsgPayload  payload_;

    // size in bytes for the payload - given by user and can't be more then the maximum 
    // message allowed by type
    size_t      payload_len_;

    // The message type
    MsgType     type_;

    // message sequence number
    uint64_t    sequence_num_;

    //protection field to assert for integrity
    long    protection_field_ = 0;

    // used to auto-assign message sequence number
    static uint64_t sequence_num_counter_;
};

}//namespace mbl

#endif // _MailboxMsg_h_
