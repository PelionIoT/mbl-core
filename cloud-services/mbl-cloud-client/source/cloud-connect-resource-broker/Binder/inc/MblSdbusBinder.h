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
extern "C" {
    #include "MblSdbusAdaptor.h"
}

namespace mbl {

/*! \file MblSdbusBinder.h
 *  \brief MblSdbusBinder.
 *  This class provides an binding and abstraction interface for a D-Bus IPC.
*/
class MblSdbusBinder : MblDBusBinder{

public:

    MblSdbusBinder();
    ~MblSdbusBinder() = default;
    
    MblError init();
    MblError de_init();
    MblError start();
    MblError stop();

private:
    enum class Status {INITALIZED, FINALIZED};
    
    Status status_ = Status::FINALIZED;

    MblSdbusCallbacks callbacks_;

    // D-BUS callbacks
    static int register_resources_callback(const char *json_filem, CCRBStatus *ccrb_status);
    static int deregister_resources_callback(const char *access_token, CCRBStatus *ccrb_status);

    // No copying or moving 
    // (see https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#cdefop-default-operations)
    MblSdbusBinder(const MblSdbusBinder&) = delete;
    MblSdbusBinder & operator = (const MblSdbusBinder&) = delete;
    MblSdbusBinder(MblSdbusBinder&&) = delete;
    MblSdbusBinder& operator = (MblSdbusBinder&&) = delete;    
};

} // namespace mbl

#endif // MblSdbusBinder_h_