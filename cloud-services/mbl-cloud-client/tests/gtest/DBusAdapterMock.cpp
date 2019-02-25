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

#include "DBusAdapterMock.h"
#include "CloudConnectTrace.h"

#define TRACE_GROUP "ccrb-debus-adapter-mock"

DBusAdapterMock::DBusAdapterMock(mbl::ResourceBroker &ccrb)
: DBusAdapter(ccrb),
reg_status_(CloudConnectStatus::STATUS_SUCCESS),
ipc_conn_handle_(0),
update_registration_called_(false)
{
    TR_DEBUG("Enter");
}
DBusAdapterMock::~DBusAdapterMock()
{
    TR_DEBUG("Enter");
}

mbl::MblError DBusAdapterMock::update_registration_status(
    const uintptr_t ipc_conn_handle, 
    const CloudConnectStatus reg_status)
{
    TR_DEBUG("Enter");

    ipc_conn_handle_ = ipc_conn_handle;
    reg_status_ = reg_status;
    update_registration_called_ = true;
    TR_DEBUG("update registration called: %d", update_registration_called_);
    return mbl::Error::None;
}

bool DBusAdapterMock::is_update_registration_called()
{
    TR_DEBUG("update registration called: %d", update_registration_called_);
    return update_registration_called_;
}

CloudConnectStatus DBusAdapterMock::get_register_cloud_connect_status()
{
    return reg_status_;
}
