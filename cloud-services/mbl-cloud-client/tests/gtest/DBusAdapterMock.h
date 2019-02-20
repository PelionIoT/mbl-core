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


#ifndef DBusAdapterMock_h_
#define DBusAdapterMock_h_

#include "cloud-connect-resource-broker/DBusAdapter.h"

/**
 * @brief Mock class for DBusAdapter
 * Using this class we can test the API communication between ResourceBroker class and DBusAdapter.
 */
class DBusAdapterMock  : public mbl::DBusAdapter {

public:

    DBusAdapterMock(mbl::ResourceBroker &ccrb);
    ~DBusAdapterMock();

    /**
     * @brief Mock function that sends registration request final status to the destination 
     * client application. 
     * 
     * @param ipc_conn_handle is a handle to the IPC unique connection information 
     *        of the application that should be notified.
     * @param reg_status status of registration of all resources. 
     *        reg_status is SUCCESS only if registration of all resources was 
     *        successfully finished, or error code otherwise.
     * 
     * @return MblError returns Error::None.
     */
    virtual mbl::MblError update_registration_status(
        const uintptr_t ipc_conn_handle, 
        const CloudConnectStatus reg_status) override;

    /**
     * @brief This function is used to check if update_registration_status was called
     * 
     * @return true if update_registration_status API was called
     * @return false if update_registration_status API was not called
     */
    bool is_update_registration_called();

    /**
     * @brief Get the register cloud connect status object from last call to 
     * update_registration_status API
     * 
     * @return CloudConnectStatus 
     */
    CloudConnectStatus get_register_cloud_connect_status();

private:

    CloudConnectStatus reg_status_;
    uintptr_t ipc_conn_handle_;
    bool update_registration_called_;
};

#endif // DBusAdapterMock_h_
