/*
 * Copyright (c) 2019 ARM Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ResourceBrokerMockBase.h"
#include "MailboxMsg.h"

#define TRACE_GROUP "ccrb-resource-broker-mock"

namespace mbl
{

void ResourceBrokerMockBase::set_ipc_adapter(DBusAdapter *adapter) {
    adapter_ = adapter;
}

MblError ResourceBrokerMockBase::process_mailbox_message(MailboxMsg& msg)
{
    TR_DEBUG_ENTER;
    assert(adapter_);
        
    auto data_type_name = msg.get_data_type_name();
    if (data_type_name == typeid(MailboxMsg_Exit).name()) {
        // EXIT message

        // validate length (sanity check).In this case the length must be equal the actual length
        if (msg.get_data_len() != sizeof(MailboxMsg_Exit)) {
            TR_ERR("Unexpected MailboxMsg_Exit message length %zu (expected %zu),"
                " returning error=%s",
                msg.get_data_len(),
                sizeof(MailboxMsg_Exit),
                MblError_to_str(MblError::DBA_MailBoxInvalidMsg));
            return Error::DBA_MailBoxInvalidMsg;
        }

        // External thread request to stop event loop
        MblError status;
        MailboxMsg_Exit message_exit;
        std::tie(status, message_exit) = msg.unpack_data<MailboxMsg_Exit>();
        TR_INFO("receive message MailboxMsg_Exit sending stop request to event loop with stop"
                " status=%s",
                MblError_to_str(message_exit.stop_status));
        if (status != MblError::None) {
            TR_ERR("msg.unpack_data failed with error %s - returing -EBADMSG",
                MblError_to_str(status));
            return Error::DBA_MailBoxInvalidMsg;
        }

        const MblError ipc_stop_err = adapter_->stop(MblError::None);
        if (Error::None != ipc_stop_err) {
            TR_ERR("ipc::stop failed! (%s)", MblError_to_str(ipc_stop_err));
            return ipc_stop_err;
        }
    }
    return Error::None;
}

MblError ResourceBrokerMockBase::send_adapter_stop_message()
{
    assert(adapter_);
    MailboxMsg_Exit message_exit;
    message_exit.stop_status = MblError::None;
    MailboxMsg msg(message_exit, sizeof(message_exit));
    return adapter_->send_mailbox_msg(msg);
}

std::pair<CloudConnectStatus, std::string> ResourceBrokerMockBase::register_resources(
    IpcConnection , 
    const std::string &)
{
    TR_DEBUG_ENTER; 
    return std::make_pair(CloudConnectStatus::STATUS_SUCCESS, "");        
}

} //namespace mbl

