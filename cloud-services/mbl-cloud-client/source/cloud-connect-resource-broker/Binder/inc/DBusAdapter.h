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


#ifndef _DBusAdapter_h_
#define _DBusAdapter_h_



#include "MblError.h"

#include "DBusAdapterMailbox.h"
extern "C" {
    #include "DBusAdapterLowLevel.h"
}

struct DBusAdapterMsg;

namespace mbl {

/*! \file MblSdbusBinder.h
 *  \brief MblSdbusBinder.
 *  This class provides an binding and abstraction interface for a D-Bus IPC.
*/
class DBusAdapter
{
public:

    DBusAdapter();
    ~DBusAdapter() = default;
    
    MblError init();
    MblError deinit();
    MblError start();
    MblError stop();

    // TODO : Use smart pointer?
    MblError mailbox_push_msg(struct DBusAdapterMsg *msg);    

private:
    enum class Status {INITALIZED, NON_INITALIZED};
    
    Status status_ = Status::NON_INITALIZED;

    MblSdbusCallbacks callbacks_;
    DBusAdapterMailbox mailbox_;
    
    // TODO : Use smart pointer?
    MblError mailbox_pop_msg(struct DBusAdapterMsg **msg);

    // D-BUS callbacks
    static int register_resources_callback(const char *json_filem, CCRBStatus *ccrb_status);
    static int deregister_resources_callback(const char *access_token, CCRBStatus *ccrb_status);

    // TODO : needed?
    // No copying or moving 
    // (see https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#cdefop-default-operations)
    DBusAdapter(const DBusAdapter&) = delete;
    DBusAdapter & operator = (const DBusAdapter&) = delete;
    DBusAdapter(DBusAdapter&&) = delete;
    DBusAdapter& operator = (DBusAdapter&&) = delete;    
};

} // namespace mbl

#endif // _DBusAdapter_h_