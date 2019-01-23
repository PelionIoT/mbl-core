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
#include "MblSdbusLowLevel.h"
}

// FIXME: uncomment later
//#include "mbed-trace/mbed_trace.h"

#define TRACE_GROUP "ccrb-dbus"

namespace mbl
{

MblError MblSdbusBinder::init()
{
    sd_bus *bus = NULL;
    MblError status;

    tr_debug(__func__);

    if (status_ == Status::INITALIZED)
    {
        return MblError::AlreadyInitialized;
    }

    if (bus_init(sdbus_.bus, sdbus_.bus_slot) != 0){
        return MblError::SdBusError;
    }

    // must be last line!
    status_ = Status::INITALIZED;
    return MblError::None;
}

MblError MblSdbusBinder::de_init()
{
    tr_debug("MblCloudConnectIpcDBus::Finalize");
    return MblError::None;
}

} // namespace mbl
