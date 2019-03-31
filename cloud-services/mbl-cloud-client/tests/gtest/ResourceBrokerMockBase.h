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


#ifndef ResourceBrokerMockBase_h_
#define ResourceBrokerMockBase_h_

#include "ResourceBroker.h"
#include "DBusAdapter.h"
#include "MblError.h"

namespace mbl {

/**
 * @brief ResourceBroker mock class
 * Using this class we can test the API communication between ResourceBroker class and DBusAdapter.
 * 
 */    
class ResourceBrokerMockBase : public ResourceBroker {

public:

    /**
     * @brief Resource broker's ipc adapter is set here
     * The adapter will call this class's process_mailbox_message API when a new mailbox msg arrived
     * @param adapter - ipc adapter
     */
    void set_ipc_adapter(DBusAdapter *adapter);

    /**
     * @brief Overrides CCRB process_mailbox_message API
     * 
     * @param msg - mailbox message to handle. currently only exit message is supported
     * @return MblError - Error::DBA_MailBoxInvalidMsg is case of unexpected mailbox msg
     *                  - Error::None - in case of success
     */
    MblError process_mailbox_message(MailboxMsg& msg) override;

    /**
     * @brief Overrides CCRB register_resources API
     * 
     * @return std::pair<CloudConnectStatus, std::string> 
     *              - Status is always CloudConnectStatus::STATUS_SUCCESS
     *              - access token is empty string
     */
    std::pair<CloudConnectStatus, std::string> 
    register_resources(IpcConnection , 
                       const std::string &) override;

    /**
     * @brief This function send stop message to mailbox in order to stop adapter
     * Mailbox will call process_mailbox_message() to handle this message
     * Note: Currently handle MailboxMsg_Exit message. if other message handing is needed - 
     * modify this function.
     */
    MblError send_adapter_stop_message();

private:

    DBusAdapter* adapter_;    
};

} // namespace mbl

#endif // ResourceBrokerMockBase_h_
