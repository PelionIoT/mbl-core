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

    void set_test_data(
        mbl::MblError register_resources_expected_err,
        CloudConnectStatus register_resources_expected_status,
        CloudConnectStatus dbus_adapter_expected_status);

    void add_objects(const M2MObjectList& object_list);
    void register_update();

    void register_resources_test(const std::string &json_string);

private:

    mbl::MblError register_resources_expected_err_;
    CloudConnectStatus register_resources_expected_status_;
    CloudConnectStatus dbus_adapter_expected_status_;

    mbl::ResourceBroker cloud_connect_resource_broker_;
};

ResourceBrokerTester::ResourceBrokerTester()
: register_resources_expected_err_(mbl::MblError::None)
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    // Set Resource Broker function pointers to point to this class instead of to Mbed Client
    // This replaces what cloud_connect_resource_broker_.init() usually does so dont call it...
    cloud_connect_resource_broker_.register_update_func_ = std::bind(&ResourceBrokerTester::register_update, this);
    cloud_connect_resource_broker_.add_objects_func_ = 
        std::bind(static_cast<void(ResourceBrokerTester::*)(const M2MObjectList&)>(&ResourceBrokerTester::add_objects),
            this,
            std::placeholders::_1);

    // Init resource broker ipc to be DBusAdapterTester:
    cloud_connect_resource_broker_.ipc_ = std::make_unique<DBusAdapterTester>(cloud_connect_resource_broker_);
}

ResourceBrokerTester::~ResourceBrokerTester()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

void ResourceBrokerTester::set_test_data(
    mbl::MblError register_resources_expected_err,
    CloudConnectStatus register_resources_expected_status,
    CloudConnectStatus dbus_adapter_expected_status)
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    register_resources_expected_err_ = register_resources_expected_err;
    register_resources_expected_status_ = register_resources_expected_status;
    dbus_adapter_expected_status_ = dbus_adapter_expected_status;
}

void ResourceBrokerTester::add_objects(const M2MObjectList& /*object_list*/)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

void ResourceBrokerTester::register_update()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

void ResourceBrokerTester::register_resources_test(const std::string &json_string)
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    const uintptr_t ipc_conn_handle = 0;
    CloudConnectStatus out_status;
    std::string out_access_token;

    mbl::MblError status = cloud_connect_resource_broker_.register_resources(
    ipc_conn_handle, 
    json_string,
    out_status,
    out_access_token);

    ASSERT_TRUE(register_resources_expected_err_ == status);
    ASSERT_TRUE(register_resources_expected_status_ == out_status);
    if(register_resources_expected_status_ != CloudConnectStatus::STATUS_SUCCESS) {
        // If we were expected to fail in register_resource API, nothing else should be tested
        return;
    }

    //Call Application Endpoint to notify registration was successful
    ASSERT_TRUE(cloud_connect_resource_broker_.app_endpoints_map_.end() !=
        cloud_connect_resource_broker_.app_endpoints_map_.find(out_access_token));
    mbl::ResourceBroker::SPApplicationEndpoint app_endpoint = 
        cloud_connect_resource_broker_.app_endpoints_map_[out_access_token];

    // Make sure application is not yet registered
    ASSERT_FALSE(app_endpoint->is_registered());

    if(dbus_adapter_expected_status_ == CloudConnectStatus::STATUS_SUCCESS) {
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

    DBusAdapterTester& dbus_adapter_tester = *(static_cast<DBusAdapterTester*>(cloud_connect_resource_broker_.ipc_.get()));
    // Verify that resource broker called the adapter
    ASSERT_TRUE(dbus_adapter_tester.is_update_registration_called());
    // Verify adapter got the right call from resource broker
    ASSERT_TRUE(dbus_adapter_expected_status_ == dbus_adapter_tester.get_register_cloud_connect_status());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(Resource_Broker_Positive, registration_success) {

    tr_debug("%s", __PRETTY_FUNCTION__);

    const std::string json_string = VALID_JSON_TWO_OBJECTS_WITH_ONE_OBJECT_INSTANCE_AND_ONE_RESOURCE;
    ResourceBrokerTester resource_broker_tester;
    resource_broker_tester.set_test_data(
        mbl::MblError::None,
        CloudConnectStatus::STATUS_SUCCESS,
        CloudConnectStatus::STATUS_SUCCESS);
    resource_broker_tester.register_resources_test(json_string);
}

TEST(Resource_Broker_Negative, parsing_succedded_registration_failed) {

    tr_debug("%s", __PRETTY_FUNCTION__);

    const std::string json_string = VALID_JSON_TWO_OBJECTS_WITH_ONE_OBJECT_INSTANCE_AND_ONE_RESOURCE;
    ResourceBrokerTester resource_broker_tester;
    resource_broker_tester.set_test_data(
        mbl::MblError::None,
        CloudConnectStatus::STATUS_SUCCESS,
        CloudConnectStatus::ERR_FAILED);
    resource_broker_tester.register_resources_test(json_string);
}

TEST(Resource_Broker_Negative, invalid_json_1) {

    tr_debug("%s", __PRETTY_FUNCTION__);

    const std::string json_string = INVALID_JSON_NOT_3_LEVEL_1;
    ResourceBrokerTester resource_broker_tester;
    resource_broker_tester.set_test_data(
        mbl::MblError::None,
        CloudConnectStatus::ERR_INVALID_JSON,
        CloudConnectStatus::ERR_INVALID_JSON);
    resource_broker_tester.register_resources_test(json_string);
}
