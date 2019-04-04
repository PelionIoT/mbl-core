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

#include "ResourceBroker.h"
#include "MbedClientManager.h"
class ConnectorClientEndpointInfo;

/**
 * @brief Mbed client mock class
 * Does almost nothing as we don't check it...
 * 
 */
class MbedClientManagerMock : public mbl::MbedClientManager
{

public:

    MbedClientManagerMock()
    : device_state_(State_DeviceUnregistered)
    {
    }

    ~MbedClientManagerMock(){}

    void init() override {}

    /**
     * @brief Deinit mbed client mock function (does nothing)
     */
    void deinit() override {}

    /**
     * @brief Set the on resources registration succeeded callback
     * 
     * @param callback_func - callback function
     */
    void set_resources_registration_succeeded_callback(
        ResourcesRegistrationSucceededCallback
    ) override {}

    /**
     * @brief Set the on Mbed client error callback
     * 
     * @param callback_func - callback function
     */
    void set_mbed_client_error_callback(MbedClientErrorCallback) override {}


    /**
     * @brief Register mbed client mock function (does nothing)
     */
    mbl::MblError register_mbed_client() override 
    {
        device_state_ = State_DeviceRegistered;
        return mbl::MblError::None;
    }

    /**
     * @brief Unregister mbed client mock function (does nothing)
     */
    void unregister_mbed_client() override 
    {
        device_state_ = State_DeviceUnregistered;
    }

    /**
     * @brief return mbed client device state
     * 
     * @return State_DeviceRegistered - mocks registration success
     */
    MbedClientDeviceState get_device_state() override 
    {
        return device_state_;
    }

    /**
     * @brief Perform keepalive to mbed client
     * 
     */
    void keepalive() override {}

    /**
     * @brief Register resources
     * 
     * @param object_list - resources to be registered
     */
    void register_resources(const M2MObjectList&) override {}

private:

    MbedClientDeviceState device_state_;    
};

/**
 * @brief This class tests ResourceBroker functionality
 * 
 * This class is a friend of ResourceBroker class
 * and is therefore able to evaluate private members and test operations
 * 
 */
class ResourceBrokerTester {

public:
    
    /**
     * @brief C'tor
     * 
     * @param use_mock_dbus_adapter - if true - init ResourceAdapter's debus_ipc to mock ipc
     */
    ResourceBrokerTester(bool use_mock_dbus_adapter = true);

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
     * @param expected_cloud_connect_status is the expected cloud connect status returned by ResourceBroker
     */
    void register_resources_test(
        const mbl::IpcConnection &source,
        const std::string& app_resource_definition,
        CloudConnectStatus& out_status,
        std::string& out_access_token,
        CloudConnectStatus expected_cloud_connect_status);

    /**
     * @brief Simulate Mbed cloud client register update callback calls
     * 
     * Depending on dbus_adapter_expected_status it either calls handle_register_cb or handle_error_cd 
     * of resource broker class to signal a success or failed registration
     * 
     * @param access_token is a token that should be used by the client 
     *        application in all APIs that access (in any way) to the provided 
     *        (via app_resource_definition_json) set of resources. 
     * @param dbus_adapter_expected_status - expected dbus adapter cloud connect status
     * 
     * Note: register_resources_test() must be called before calling this API.
     */
    void mbed_client_register_update_callback_test(
        const std::string& access_token,
        CloudConnectStatus dbus_adapter_expected_status);

    /**
     * @brief This function create a new thread that simulate mbed client thread
     * and pass the expected behaviour of mbed client thread (registration fail or success)
     * Then it check the result of this mbed client simulation in terms of expected result in
     * Resource broker.
     * 
     * @param access_token - access token of the registration operation
     * @param simulate_registration_success - if true - simulate success
     *                                        if false - simulate failure
     */
    void simulate_mbed_client_register_update_callback_test(
        const std::string& access_token,
        bool simulate_registration_success);

    /**
     * @brief Get resource by path and compare to expected status
     * 
     * @param access_token is a token that should be used by the client 
     *        application in all APIs that access (in any way) to the provided 
     *        (via app_resource_definition_json) set of resources. 
     * @param path - resource path
     * @param expected_error_status - expected error status
     * 
     * Note: register_resources_test() must be called before calling this API.
     */
    void get_m2m_resource_test(
        const std::string& access_token,
        const std::string& path,
        mbl::MblError expected_error_status);

    /**
     * @brief Test set_resources_values API
     * 
     * @param access_token is a token that should be used by the client 
     *        application in all APIs that access (in any way) to the provided 
     *        (via app_resource_definition_json) set of resources. 
     * @param inout_set_operations vector of structures that provide all input and 
     *        output parameters to perform setting operation. 
     *        Each entry in inout_set_operations contains:
     * 
     *        input fields: 
     *        - input_data is the data that includes resources's path type and value 
     *          of the corresponding resource.
     * 
     *        output field: 
     *        - output_status is the status of the set operation for the corresponding 
     *          resource.
     *        Note: This parameter is valid, if MblError return error code 
     *        was Error::None.  
     * @param expected_inout_set_operations - expected inout_set operation vector that contains
     *        the same resources as in inout_set_operations vector, and it includes the expected
     *        cloud connect status for comparison.
     *        Note: the order of the resources in this vector should be exactly in the same order as 
     *        in inout_set_operations, and the same number of items or the test will fail.
     * 
     * @param expected_out_status cloud connect operation status for operations like 
     *        access_token validity, access permissions to the resources, and so on. 
     *        fills corresponding value to the output_status in inout_set_operations.
     * 
     * Note: register_resources_test() must be called before calling this API.
     */
    void set_resources_values_test(
        const std::string& access_token,
        std::vector<mbl::ResourceSetOperation>& inout_set_operations,
        const std::vector<mbl::ResourceSetOperation> expected_inout_set_operations,
        const CloudConnectStatus expected_out_status);

    /**
     * @brief Test get_resources_values API
     * 
     * @param access_token is a token that should be used by the client 
     *        application in all APIs that access (in any way) to the provided 
     *        (via app_resource_definition_json) set of resources. 
     * @param inout_get_operations vector of structures that provide all input and 
     *        output parameters required to perform getting operation. 
     *        Each entry in inout_get_operations contains:
     * 
     *        input fields: 
     *        - inout_data.path field is the path of the corresponding resource 
     *          who's value should be gotten. 
     *        - inout_data.type field is the type of the resource data.
     * 
     *        output fields: 
     *        - output_status is the status of the set operation for the corresponding
     *          resource.
     *        - inout_data.value field is the value that was gotten from resource. 
     *          Use inout_data.value only if the output_status has SUCCESS value. 
     *        Note: This parameter is valid, if MblError return error code 
     *        was Error::None.  
     * @param expected_inout_get_operations - expected inout_get operation vector that contains
     *        the same resources as in inout_get_operations vector, and it includes the expected
     *        resource values / cloud connect status for comparison.
     *        Note: the order of the resources in this vector should be exactly in the same order as 
     *        in inout_get_operations, and the same number of items or the test will fail.
     * @param expected_out_status cloud connect operation status for operations like 
     *        access_token validity, access permissions to the resources, and so on. 
     *        fills corresponding value to the output_status in inout_set_operations.
     * 
     * Note: register_resources_test() and set_Resource_value_test() must be called before 
     * calling this API.
     */
    void get_resources_values_test(
        const std::string& access_token,
        std::vector<mbl::ResourceGetOperation>& inout_get_operations,
        const std::vector<mbl::ResourceGetOperation> expected_inout_get_operations,
        const CloudConnectStatus expected_out_status);
		
    /**
     * @brief Call ResourceBroker start and then stop functions several times
     * 
     * @param times to call ResourceBroker start and then stop functions
     */
    void resourceBroker_start_stop_test(size_t times);		

    /**
     * @brief Test that ResourceBroker track IPC connections as expected
     * ResourceBroker uses 3 registration records:
     * 1. Registration record 1 with IPC connections: source_1 and source_2
     * 2. Registration record 2 with IPC connections: source_2 and source_1
     * 3. Registration record 3 with IPC connections: source_3
     * 
     * Close connection source_3 and make sure ResourceBroker erased registration record 3 as it
     * does not have any valid connections anymore
     * Close connection source_1 - Broker have 2 registration records
     * Close connection source_2 - Broker have no registration records.
     */
    void notify_connection_closed_test_multiple_reg_records();

    /**
     * @brief Call ccrb_.notify_connection_closed() API
     * 
     * @param source - connection which has been closed
     */
    void notify_connection_closed(mbl::IpcConnection source);

    /**
     * @brief Return internal ccrb
     * This function is used for tests that need ccrb reference as they can't create it
     * because ccrb c'tor is private (this class is friend of it so it can be done)
     * 
     * @return mbl::ResourceBroker& 
     */
    inline mbl::ResourceBroker& get_ccrb() {return ccrb_;}

    //TODO: DOCS
    static void* resource_broker_main_thread_func(void* tester);

    /**
     * @brief Start ccrb thread and init adapter
     * 
     */
    pthread_t start_ccrb();

    //TODO: DOCS
    /**
     * @brief Stop ccrb thread and deinit adapter
     * 
     */
    void stop_ccrb(pthread_t resource_broker_main_thread_id);

private:

    /**
     * @brief Mbed client mock thread that simulates calls to resource broker's callback functions
     * Based on expected behaviour input thread params
     * 
     * @param data - struct passed from thread creator that includes pointer to tester and expected
     *               registration result (fail/success). based on that data the right callback will
     *               be called
     * @return void* - thread result
     */
    static void* mbed_client_mock_thread_func(void* data);

    /**
     * @brief ResourceBroker module to be tested
     * 
     */
    mbl::ResourceBroker ccrb_;
};

#endif // ResourceBrokerTester_h_
