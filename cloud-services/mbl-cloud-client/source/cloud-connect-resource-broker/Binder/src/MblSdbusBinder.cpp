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

extern "C" {
#include <systemd/sd-bus.h>
#include "MblSdbusAdaptor.h"
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

    if (Status::INITALIZED == status_ )
    {
        return MblError::AlreadyInitialized;
    }
    
    if (SdBusAdaptor_init(&callbacks_) != 0){
        return MblError::SdBusError;
    }

    // initalize mailbox
    // must be last line!
    status_ = Status::INITALIZED;
    return MblError::None;
}

MblError MblSdbusBinder::de_init()
{
    tr_debug(__PRETTY_FUNCTION__);

    status_ = Status::FINALIZED;
    return MblError::None;
}

MblError MblSdbusBinder::start()
{
    if (status_ != Status::INITALIZED){
        return MblError::NotInitialized;
    }

    int32_t r = SdBusAdaptor_run();
    if (r != 0) {
        return MblError::SdBusError;
    }

    return MblError::None;
}

MblError MblSdbusBinder::stop()
{
    if (status_ != Status::INITALIZED){
        return MblError::NotInitialized;
    }    
}

} // namespace mbl
