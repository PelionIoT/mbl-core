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

#include "MblSdbusBinder.h"

extern "C"
{
#include <systemd/sd-bus.h>
#include "MblSdbusAdaptor.h"
#include "MblSdbusPipe.h"
}

#include <string>

// FIXME: uncomment later
//#include "mbed-trace/mbed_trace.h"

#define TRACE_GROUP "ccrb-dbus"

namespace mbl
{

int MblSdbusBinder::register_resources_callback(const char *json_file, CCRBStatus *ccrb_status)
{
    std::string json(json_file);
    return 0;
}

int MblSdbusBinder::deregister_resources_callback(const char *access_token, CCRBStatus *ccrb_status)
{
    std::string json(access_token);
    return 0;
}

MblSdbusBinder::MblSdbusBinder()
{
    tr_debug(__PRETTY_FUNCTION__);

    // set callbacks
    callbacks_.register_resources_callback = register_resources_callback;
    callbacks_.deregister_resources_callback = deregister_resources_callback;
}

MblError MblSdbusBinder::init()
{
    tr_debug(__PRETTY_FUNCTION__);

    if (Status::INITALIZED == status_)
    {
        return MblError::AlreadyInitialized;
    }

    if (SdBusAdaptor_init(&callbacks_) != 0)
    {
        return MblError::SdBusError;
    }

    if (MblSdbusPipe_create(&this->mailbox_) != 0)
    {
        return MblError::SdBusError;
    }

    status_ = Status::INITALIZED;
    return MblError::None;
}

// FIXME = do best effort
MblError MblSdbusBinder::deinit()
{
    tr_debug(__PRETTY_FUNCTION__);

    if (SdBusAdaptor_deinit() != 0)
    {
        return MblError::SdBusError;
    }

    if (MblSdbusPipe_destroy(&mailbox_) != 0)
    {
        return MblError::SdBusError;
    }

    status_ = Status::FINALIZED;
    return MblError::None;
}

MblError MblSdbusBinder::start()
{
    if (status_ != Status::INITALIZED)
    {
        return MblError::NotInitialized;
    }

    int32_t r = SdBusAdaptor_run();
    if (r != 0)
    {
        return MblError::SdBusError;
    }

    return MblError::None;
}

MblError MblSdbusBinder::stop()
{
    if (status_ != Status::INITALIZED)
    {
        return MblError::NotInitialized;
    }
}

// TODO : Cconsider implementing overloading - mailbox_send_msg
// TODO : check when need to free
MblError MblSdbusBinder::mailbox_push_msg(MblSdbusPipeMsg *msg)
{
    MblSdbusPipeMsg *msg_;
    int r;

    if (nullptr == msg){
        return MblError::CCRBStartFailed;
    }
    if (msg->type >= PIPE_MSG_TYPE_LAST)
    {
        return MblError::CCRBStartFailed;
    }

    msg_ = (MblSdbusPipeMsg *)malloc(sizeof(MblSdbusPipeMsg));
    if (NULL == msg_)
    {
        return MblError::CCRBStartFailed;
    }
    memcpy(msg_, msg, sizeof(MblSdbusPipeMsg));
    r = MblSdbusPipe_data_send(mailbox_, msg_);
    if (r < 0){
        return MblError::CCRBStartFailed;
    }
    return MblError::None;
}


//TODO : check when need to free
MblError MblSdbusBinder::mailbox_pop_msg(MblSdbusPipeMsg **msg)
{
    MblSdbusPipeMsg *msg_;

    if (nullptr == msg){
        return MblError::CCRBStartFailed;
    }

    r = MblSdbusPipe_data_receive(mailbox_, &msg_);
    if (r < 0){
        return MblError::CCRBStartFailed;
    }

    if (msg_->type >= PIPE_MSG_TYPE_LAST)
    {
        // TODO - free?
        return MblError::CCRBStartFailed;
    }
    *msg = msg_;
    return MblError::None;
}

} // namespace mbl
