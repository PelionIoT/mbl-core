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

#include <string>

#include "DBusAdapter.h"
#include "DBusAdapterMsg.h"
#include "MblError.h"


#include "DBusAdapterMailbox.h"
extern "C"
{
    #include <systemd/sd-bus.h>
    #include "DBusAdapterLowLevel.h"
}

#define TRACE_GROUP "ccrb-dbus"

namespace mbl
{
/*
int DBusAdapter::register_resources_callback(const char *json_file, CCRBStatus *ccrb_status)
{
    std::string json(json_file);
    return 0;
}

int DBusAdapter::deregister_resources_callback(const char *access_token, CCRBStatus *ccrb_status)
{
    std::string json(access_token);
    return 0;
}

DBusAdapter::DBusAdapter()
{
    tr_debug(__PRETTY_FUNCTION__);

    // set callbacks
    callbacks_.register_resources_callback = register_resources_callback;
    callbacks_.deregister_resources_callback = deregister_resources_callback;
}

MblError DBusAdapter::init()
{
    tr_debug(__PRETTY_FUNCTION__);

    if (Status::INITALIZED == status_)
    {
        return MblError::AlreadyInitialized;
    }

    if (DBusAdapterLowLevel_init(&callbacks_) != 0)
    {
        return MblError::DBusErr_Temporary;
    }

    if (DBusAdapterMailbox_alloc(&mailbox_) != 0)
    {
        return MblError::DBusErr_Temporary;
    }

    status_ = Status::INITALIZED;
    return MblError::None;
}

// FIXME = do best effort
MblError DBusAdapter::deinit()
{
    tr_debug(__PRETTY_FUNCTION__);

    if (DBusAdapterLowLevel_deinit() != 0)
    {
        return MblError::DBusErr_Temporary;
    }

    if (DBusAdapterMailbox_free(&mailbox_) != 0)
    {
        return MblError::DBusErr_Temporary;
    }

    status_ = Status::NON_INITALIZED;
    return MblError::None;
}

MblError DBusAdapter::start()
{
    if (status_ != Status::INITALIZED)
    {
        return MblError::NotInitialized;
    }

    int32_t r = DBusAdapterLowLevel_run();
    if (r != 0)
    {
        return MblError::DBusErr_Temporary;
    }

    return MblError::None;
}

MblError DBusAdapter::stop()
{
    if (status_ != Status::INITALIZED)
    {
        return MblError::NotInitialized;
    }
}

// TODO : check when need to free
MblError DBusAdapter::mailbox_push_msg(struct DBusAdapterMsg *msg)
{
    struct DBusAdapterMsg *msg_;
    int r;

    if (nullptr == msg){
        return MblError::DBusErr_Temporary;
    }
    if (msg->type >= DBUS_ADAPTER_MSG_LAST)
    {
        return MblError::DBusErr_Temporary;
    }

    msg_ = (DBusAdapterMsg *)malloc(sizeof(DBusAdapterMsg));
    if (NULL == msg_)
    {
        return MblError::DBusErr_Temporary;
    }
    
    memcpy(msg_, msg, sizeof(DBusAdapterMsg));
    r = DBusAdapterMailbox_send(&mailbox_, reinterpret_cast<std::uintptr_t>(msg_));
    if (r < 0){
        return MblError::DBusErr_Temporary;
    }
    return MblError::None;
}


//TODO : check when need to free
MblError DBusAdapter::mailbox_pop_msg(struct DBusAdapterMsg **msg)
{
    int r;
    uintptr_t ptr;
    struct DBusAdapterMsg *msg_;

    if (nullptr == msg){
        return MblError::DBusErr_Temporary;
    }

    r = DBusAdapterMailbox_ptr_receive(&mailbox_, &ptr);
    if (r < 0){
        return MblError::DBusErr_Temporary;
    }

    msg_ = reinterpret_cast<DBusAdapterMsg*>(ptr);

    if (msg_->type >= DBUS_ADAPTER_MSG_LAST)
    {
        // TODO - free?
        return MblError::DBusErr_Temporary;
    }
    
    *msg = msg_;

    return MblError::None;
}
*/
} // namespace mbl
