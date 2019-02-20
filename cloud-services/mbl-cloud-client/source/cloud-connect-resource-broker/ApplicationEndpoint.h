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

/**
 * @brief This class represent an Application endpoint, holds M2M resources, access token and more.
 * This class register for Mbed cloud client callbacks, and when called - pass the relevant information
 * to ResourceBroker.
 */
class ApplicationEndpoint {

friend ::ResourceBrokerTester;
friend ResourceBroker;

public:

    ApplicationEndpoint(const uintptr_t ipc_conn_handle, std::string access_token, ResourceBroker &ccrb);
    ~ApplicationEndpoint();

    /**
     * @brief Initialize Application resource M2M lists using JSON string
     * 
     * @param json_string - Application resource definition JSON string
     * @return MblError -
     *      Error::None - If function succeeded
     *      Error::CCRBInvalidJson - I case of invalid JSON (e.g. Invalid JSON structure or invalid M2M content such as missing mandatory entries)
     *      Error::CCRBCreateM2MObjFailed - If create of M2M object/object instance/resource failed
     */
    MblError init(const std::string json_string);

private:

    /**
     * @brief Resitration update callback
     * When registration flow is finished - Mbed cloud client will call this callback.
     * This function will notify the Resource broker that its registration finished successfully.
     */
    void handle_registration_updated_cb();

    /**
     * @brief Error callback
     * Mbed cloud client will call this callback when an error occurred (e.g. during registration).
     * This function will notify the Resource broker that an error occurred.
     */
    void handle_error_cb(const int cloud_client_code);

    uintptr_t ipc_conn_handle_;
    std::string access_token_;
    ResourceBroker &ccrb_; // Resource Broker to be updated whenever a callback is being called
    bool registered_;
    M2MObjectList m2m_object_list_; // Cloud client M2M object list used for registration
    RBM2MObjectList rbm2m_object_list_;
};

} // namespace mbl

#endif // ApplicationEndpoint_h_
