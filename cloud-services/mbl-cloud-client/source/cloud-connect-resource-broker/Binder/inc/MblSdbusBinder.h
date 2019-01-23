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


#ifndef MblSdbusBinder_h_
#define MblSdbusBinder_h_

#include "MblDBusBinder.h"
#include "systemd/sd-bus.h"
#include <mutex>

namespace mbl {

/*! \file MblSdbusBinder.h
 *  \brief MblSdbusBinder.
 *  This class provides an binding and abstraction interface for a D-Bus IPC.
*/
class MblSdbusBinder : MblDBusBinder{

public:

    MblSdbusBinder() = default;
    ~MblSdbusBinder() = default;
    
    MblError init();
    MblError de_init();

    // DBUS callbacks
    static int RegisterResourcesHandler(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);

private:
    enum class Status {INITALIZED, FINALIZED};
    struct MblSdbus {
        sd_bus *bus = nullptr;
        sd_bus_slot *bus_slot= nullptr;         // TODO : needed?
    };
    
    struct MblSdbus sdbus_;
    std::mutex mtx_;    
    Status status_ = Status::FINALIZED;

    // No copying or moving (see https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#cdefop-default-operations)
    MblSdbusBinder(const MblSdbusBinder&) = delete;
    MblSdbusBinder & operator = (const MblSdbusBinder&) = delete;
    MblSdbusBinder(MblSdbusBinder&&) = delete;
    MblSdbusBinder& operator = (MblSdbusBinder&&) = delete;    
};

} // namespace mbl

#endif // MblSdbusBinder_h_