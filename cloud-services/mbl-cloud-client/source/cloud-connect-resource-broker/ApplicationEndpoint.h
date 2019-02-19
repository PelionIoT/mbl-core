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


#ifndef ApplicationEndpoint_h_
#define ApplicationEndpoint_h_

#include "M2MResourceObjects.h"
#include <string>
#include <memory>

class ResourceBrokerTester;

namespace mbl {

class ResourceBroker;

class ApplicationEndpoint {

friend ::ResourceBrokerTester;
friend ResourceBroker;

public:

    ApplicationEndpoint(const uintptr_t ipc_conn_handle, std::string access_token, ResourceBroker &ccrb);
    ~ApplicationEndpoint();

    MblError init(const std::string json_string);

    void update_ipc_conn_handle(const uintptr_t ipc_conn_handle);

    uintptr_t get_ipc_conn_handle() const;

    std::string get_access_token() const;

    /**
     * @brief Resitration callback
     * When registration flow is finished - Mbed cloud client will call this callback.
     * This function will notify the Resource broker that its registration finished successfully.
     */
    void handle_register_cb();

    /**
     * @brief Error callback
     * When error occurred (e.g. during registration) - Mbed cloud client will call this callback.
     * This function will notify the Resource broker that error occurred.
     */
    void handle_error_cb(const int cloud_client_code);

    bool is_registered();

    M2MObjectList& get_m2m_object_list();

private:

    uintptr_t ipc_conn_handle_;
    std::string access_token_;

    // this class must have a reference that should be always valid to the CCRB instance. 
    // reference class member satisfy this condition.   
    ResourceBroker &ccrb_;

    bool registered_; //TODO: use a more descriptive states (e.g. for deregister as well?)

    M2MObjectList m2m_object_list_;
    RBM2MObjectList rbm2m_object_list_;
};

} // namespace mbl

#endif // ApplicationEndpoint_h_
