/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "MailboxMsg.h"
#include "CloudConnectTrace.h"
#include "CloudConnectTypes.h"

#define TRACE_GROUP "ccrb-mailbox"

namespace mbl {

uint64_t MailboxMsg::sequence_num_counter_ = 1;

MailboxMsg::MailboxMsg(MsgType type, MsgPayload &payload, size_t payload_len) :
    payload_(payload),
    payload_len_(payload_len),
    type_(type),   
    sequence_num_(sequence_num_counter_++),
    protection_field_(MSG_PROTECTION_FIELD) 
{
    TR_DEBUG("Enter");
}

MailboxMsg::MailboxMsg() :
    payload_{},
    payload_len_(0),
    type_(MsgType::UNKNOWN),
    sequence_num_(0),
    protection_field_(MSG_PROTECTION_FIELD)
{
    TR_DEBUG("Enter");  
}

const char* MailboxMsg::MsgType_to_str()
{
    TR_DEBUG("Enter");
    switch (type_)
    {
        RETURN_STRINGIFIED_VALUE( MsgType::UNKNOWN);
        RETURN_STRINGIFIED_VALUE( MsgType::EXIT);
        RETURN_STRINGIFIED_VALUE( MsgType::RAW_DATA);
        default: return "Invalid Message Type!";
    }
}

} //namespace mbl

