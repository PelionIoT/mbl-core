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


#ifndef ResourceBrokerTester_h_
#define ResourceBrokerTester_h_

#include "cloud-connect-resource-broker/ResourceBroker.h"

/**
 * @brief This class tests ResourceBroker functionality
 * 
 * This class is a friend of ResourceBroker and ApplicationEndpoint classes
 * and is therefore able to evaluate private members and test operations
 * 
 */
class ResourceBrokerTester {

public:
    
    ResourceBrokerTester();
    ~ResourceBrokerTester();

    /**
     * @brief Call ResourceBroker register_resources API
     * verify expected return value and cloud connect status returned by this API
     * 
     * @param ipc_conn_handle handle to the IPC unique connection information 
     *        of the application that should get update_registration_status message.
     * @param app_resource_definition_json json string that describes resources 
     *        that should be registered. The structure of the JSON document 
     *        reflects the structure of the required resource tree. 
     * @param out_status cloud connect operation status for operations like 
     *        json file structure validity, sending registration request 
     *        to the Cloud, and so on. 
     *        Note: This parameter is valid, if MblError return error code 
     *        was Error::None.  
     * @param out_access_token is a token that should be used by the client 
     *        application in all APIs that access (in any way) to the provided 
     *        (via app_resource_definition_json) set of resources. 
     *        Note: This parameter is valid, if MblError return error code 
     *        was Error::None.  
     * @param expected_error_status is the expected error status returned by ResourceBroker
     * @param expected_cloud_connect_status is the expected cloud connect status returned by ResourceBroker
     */
    void register_resources_test(
        const uintptr_t ipc_conn_handle,
        const std::string& app_resource_definition_json,
        CloudConnectStatus& out_status,
        std::string& out_access_token,
        mbl::MblError expected_error_status,
        CloudConnectStatus expected_cloud_connect_status);

    /**
     * @brief Simulate Mbed cloud client register update callback calls
     * 
     * Depending on dbus_adapter_expected_status it either calls handle_register_cb or handle_error_cd 
     * of ApplicationEnpoint class to signal a success or failed registration
     * 
     * @param access_token is a token that should be used by the client 
     *        application in all APIs that access (in any way) to the provided 
     *        (via app_resource_definition_json) set of resources. 
     * @param dbus_adapter_expected_status - expected dbus adapter cloud connect status
     */
    void mbed_client_register_update_callback_test(
        const std::string& access_token,
        CloudConnectStatus dbus_adapter_expected_status);

private:

    /**
     * @brief Mock Mbed client function that ads a list of objects that the application wants to 
     * register to the LWM2M server.
     * 
     * This function is used tp override the function pointer in ResourceBroker so the call will be sent 
     * to this class instead of to Mbed cloud client.
     * 
     * @param object_list - Objects that contain information about the
     * client attempting to register to the LWM2M server.
     */
    void add_objects(const M2MObjectList& object_list);

    /**
     * \brief Mock Mbed client function that sends a registration update message to the Cloud.
     * 
     * This function is used tp override the function pointer in ResourceBroker so the call will be sent 
     * to this class instead of to Mbed cloud client.
     */
    void register_update();

    /**
     * @brief ResourceBroker module to be tested
     * 
     */
    mbl::ResourceBroker cloud_connect_resource_broker_;
};

#endif // ResourceBrokerTester_h_
