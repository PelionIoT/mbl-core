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

#include <gtest/gtest.h>

#include "ResourceDefinitionJson.h"
#include "cloud-connect-resource-broker/ResourceBroker.h"
#include "cloud-connect-resource-broker/DBusAdapter.h"
#include "MblError.h"
#include "log.h"
#include "mbed-trace/mbed_trace.h"

#define TRACE_GROUP "ccrb-resource-broker-test"

////////////////////////////////////////////////////////////////////////////////////////////////////
class DBusAdapterTester  : public mbl::DBusAdapter {

public:

    DBusAdapterTester(mbl::ResourceBroker &ccrb);
    ~DBusAdapterTester();

    virtual mbl::MblError update_registration_status(
        const uintptr_t ipc_conn_handle, 
        const CloudConnectStatus reg_status) override;

    bool is_update_registration_called();
    CloudConnectStatus get_register_cloud_connect_status();

private:

    CloudConnectStatus reg_status_;
    uintptr_t ipc_conn_handle_;
    bool update_registration_called_;

};

DBusAdapterTester::DBusAdapterTester(mbl::ResourceBroker &ccrb)
: DBusAdapter(ccrb),
reg_status_(CloudConnectStatus::STATUS_SUCCESS),
ipc_conn_handle_(0),
update_registration_called_(false)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}
DBusAdapterTester::~DBusAdapterTester()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

mbl::MblError DBusAdapterTester::update_registration_status(
    const uintptr_t ipc_conn_handle, 
    const CloudConnectStatus reg_status)
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    ipc_conn_handle_ = ipc_conn_handle;
    reg_status_ = reg_status;
    update_registration_called_ = true;
    tr_debug("%s: update registration called: %d", __PRETTY_FUNCTION__, update_registration_called_);
    return mbl::Error::None;
}

bool DBusAdapterTester::is_update_registration_called()
{
    tr_debug("%s: update registration called: %d", __PRETTY_FUNCTION__, update_registration_called_);
    return update_registration_called_;
}

CloudConnectStatus DBusAdapterTester::get_register_cloud_connect_status()
{
    return reg_status_;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
class ResourceBrokerTester {

public:
    
    ResourceBrokerTester();
    ~ResourceBrokerTester();

    void register_resources_test(
        const uintptr_t ipc_conn_handle,
        const std::string& app_resource_definition_json,
        CloudConnectStatus& out_status,
        std::string& out_access_token,
        mbl::MblError expected_error_status,
        CloudConnectStatus expected_cloud_connect_status);

    void mbed_client_callback_test(
        const std::string& out_access_token,
        CloudConnectStatus dbus_adapter_expected_status);

    void add_objects(const M2MObjectList& object_list); //PRIVATE?
    void register_update(); //PRIVATE?

private:

    mbl::ResourceBroker cloud_connect_resource_broker_;
};

ResourceBrokerTester::ResourceBrokerTester()
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    // Set Resource Broker function pointers to point to this class instead of to Mbed Client
    // This replaces what cloud_connect_resource_broker_.init() usually does so dont call it...
    cloud_connect_resource_broker_.register_update_func_ = 
        std::bind(&ResourceBrokerTester::register_update, this);
    cloud_connect_resource_broker_.add_objects_func_ = 
        std::bind(static_cast<void(ResourceBrokerTester::*)(const M2MObjectList&)>(&ResourceBrokerTester::add_objects),
            this,
            std::placeholders::_1);

    // Init resource broker ipc to be DBusAdapterTester:
    cloud_connect_resource_broker_.ipc_ = 
        std::make_unique<DBusAdapterTester>(cloud_connect_resource_broker_);
}

ResourceBrokerTester::~ResourceBrokerTester()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

void ResourceBrokerTester::add_objects(const M2MObjectList& /*object_list*/)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

void ResourceBrokerTester::register_update()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

void ResourceBrokerTester::mbed_client_callback_test(
    const std::string& out_access_token,
    CloudConnectStatus dbus_adapter_expected_status)
{
    //Call Application Endpoint to notify registration was successful
    ASSERT_TRUE(cloud_connect_resource_broker_.app_endpoints_map_.end() !=
        cloud_connect_resource_broker_.app_endpoints_map_.find(out_access_token));
    mbl::ResourceBroker::SPApplicationEndpoint app_endpoint = 
        cloud_connect_resource_broker_.app_endpoints_map_[out_access_token];

    // Make sure application is not yet registered
    ASSERT_FALSE(app_endpoint->is_registered());

    if(dbus_adapter_expected_status == CloudConnectStatus::STATUS_SUCCESS) {
        // Check registration success flow
        tr_debug("%s: Notify Application (access_token: %s) that registration was successful",
            __PRETTY_FUNCTION__,
            out_access_token.c_str());

        // Next call will invoke resource-broker's cb function with will then call DBusAdapterTest
        app_endpoint->handle_register_cb();
        ASSERT_TRUE(app_endpoint->is_registered());
    } else {
        // Check registration failure flow
        tr_debug("%s: Notify Application (access_token: %s) that registration failed",
            __PRETTY_FUNCTION__,
            out_access_token.c_str());

        // Next call will invoke resource-broker's cb function with will then call DBusAdapterTest
        app_endpoint->handle_error_cb(MbedCloudClient::ConnectInvalidParameters);
        ASSERT_FALSE(app_endpoint->is_registered());
    }

    DBusAdapterTester& dbus_adapter_tester = 
        *(static_cast<DBusAdapterTester*>(cloud_connect_resource_broker_.ipc_.get()));
    // Verify that resource broker called the adapter (both for success and failure)
    ASSERT_TRUE(dbus_adapter_tester.is_update_registration_called());
    // Verify adapter got the right call from resource broker
    ASSERT_TRUE(dbus_adapter_expected_status == 
        dbus_adapter_tester.get_register_cloud_connect_status());
}

void ResourceBrokerTester::register_resources_test(
    const uintptr_t ipc_conn_handle,
    const std::string& app_resource_definition_json,
    CloudConnectStatus& out_status,
    std::string& out_access_token,
    mbl::MblError expected_error_status,
    CloudConnectStatus expected_cloud_connect_status)
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    mbl::MblError status = cloud_connect_resource_broker_.register_resources(
        ipc_conn_handle, 
        app_resource_definition_json,
        out_status,
        out_access_token);

    ASSERT_TRUE(expected_error_status == status); // Check return code from the function
    ASSERT_TRUE(expected_cloud_connect_status == out_status); // Check cloud connect expected status
}

////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief This test successful registration
 * 
 * The tested scenario is:
 * 1. Resource Broker is called for register_resources() API
 * 2. JSON is parsed by ApplicationEndpoint class
 * 3. Resource Broker registers ApplicationEndpoint's callbacks so MBed client will call them
 * 4. Resource Broker calls Mbed cloud client to register the resources
 * 5. Mbed cloud client calls ApplicationEndpoint's REGISTER callback upon SUCCESSFUL registration
 * 6. ApplicationEndpoint notify the Resource Broker that registration was SUCCESSFUL
 * 7. Resource Broker notifys DbusAdapter that registration was SUCCESSFUL
 */
TEST(Resource_Broker_Positive, registration_success) {

    tr_debug("%s", __PRETTY_FUNCTION__);

    ResourceBrokerTester resource_broker_tester;
    const uintptr_t ipc_conn_handle = 0;
    const std::string json_string = VALID_JSON_TWO_OBJECTS_WITH_ONE_OBJECT_INSTANCE_AND_ONE_RESOURCE;
    CloudConnectStatus cloud_connect_out_status;
    std::string out_access_token;

    resource_broker_tester.register_resources_test(
        ipc_conn_handle,
        json_string,
        cloud_connect_out_status,
        out_access_token,
        mbl::MblError::None, //expected error status
        CloudConnectStatus::STATUS_SUCCESS //expected cloud connect status
    );

    // Test registration callback succeeded
    resource_broker_tester.mbed_client_callback_test(
        out_access_token,
        CloudConnectStatus::STATUS_SUCCESS);
}

/**
 * @brief This test failed registration
 * 
 * The tested scenario is:
 * 1. Resource Broker is called for register_resources() API
 * 2. JSON is parsed by ApplicationEndpoint class
 * 3. Resource Broker registers ApplicationEndpoint's callbacks so MBed client will call them
 * 4. Resource Broker calls Mbed cloud client to register the resources
 * 5. Mbed cloud client calls ApplicationEndpoint's ERROR callbacks upon FAILED registration
 * 6. ApplicationEndpoint notify the Resource Broker that registration FAILED
 * 7. Resource Broker notifys DbusAdapter that registration was FAILED
 */
TEST(Resource_Broker_Negative, parsing_succedded_registration_failed) {

    tr_debug("%s", __PRETTY_FUNCTION__);

    ResourceBrokerTester resource_broker_tester;
    const uintptr_t ipc_conn_handle = 0;
    const std::string json_string = VALID_JSON_TWO_OBJECTS_WITH_ONE_OBJECT_INSTANCE_AND_ONE_RESOURCE;
    CloudConnectStatus cloud_connect_out_status;
    std::string out_access_token;

    resource_broker_tester.register_resources_test(
        ipc_conn_handle,
        json_string,
        cloud_connect_out_status,
        out_access_token,
        mbl::MblError::None, //expected error status
        CloudConnectStatus::STATUS_SUCCESS //expected cloud connect status
    );

    // Test registration callback failure
    resource_broker_tester.mbed_client_callback_test(
        out_access_token,
        CloudConnectStatus::ERR_FAILED);
}

/**
 * @brief This test failed registration
 * 
 * The tested scenario is:
 * 1. Resource Broker is called for register_resources() API
 * 2. JSON is parsed by ApplicationEndpoint class but FAIL due to invalid JSON
 * 3. Resource Broker return the FAILED registration cloud client status to the adapter
 */
TEST(Resource_Broker_Negative, invalid_json_1) {

    tr_debug("%s", __PRETTY_FUNCTION__);

    ResourceBrokerTester resource_broker_tester;
    const uintptr_t ipc_conn_handle = 0;
    const std::string json_string = INVALID_JSON_NOT_3_LEVEL_1;
    CloudConnectStatus cloud_connect_out_status;
    std::string out_access_token;

    resource_broker_tester.register_resources_test(
        ipc_conn_handle,
        json_string,
        cloud_connect_out_status,
        out_access_token,
        mbl::MblError::None, //expected error status
        CloudConnectStatus::ERR_INVALID_JSON //expected cloud connect status
    );
}

//NOTE: this test is valid only for Single app support
TEST(Resource_Broker_Negative, already_registered) {

    tr_debug("%s", __PRETTY_FUNCTION__);

    ResourceBrokerTester resource_broker_tester;
    const uintptr_t ipc_conn_handle_1 = 1;
    const std::string json_string_1 = VALID_JSON_OBJECT_WITH_SEVERAL_OBJECT_INSTANCES_AND_RESOURCES;
    CloudConnectStatus cloud_connect_out_status_1;
    std::string out_access_token_1;

    resource_broker_tester.register_resources_test(
        ipc_conn_handle_1,
        json_string_1,
        cloud_connect_out_status_1,
        out_access_token_1,
        mbl::MblError::None, //expected error status
        CloudConnectStatus::STATUS_SUCCESS //expected cloud connect status
    );

    // Test registration callback succeeded
    resource_broker_tester.mbed_client_callback_test(
        out_access_token_1,
        CloudConnectStatus::STATUS_SUCCESS);

    //Try another register - expect fail
    const uintptr_t ipc_conn_handle_2 = 2;
    const std::string json_string_2 = VALID_JSON_TWO_OBJECTS_WITH_ONE_OBJECT_INSTANCE_AND_ONE_RESOURCE;
    CloudConnectStatus cloud_connect_out_status_2;
    std::string out_access_token_2;

    resource_broker_tester.register_resources_test(
        ipc_conn_handle_2,
        json_string_2,
        cloud_connect_out_status_2,
        out_access_token_2,
        mbl::MblError::None, //expected error status
        CloudConnectStatus::ERR_ALREADY_REGISTERED //expected cloud connect status
    );    
}