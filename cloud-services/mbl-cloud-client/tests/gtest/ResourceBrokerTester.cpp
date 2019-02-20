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

#include "ResourceBrokerTester.h"
#include "DBusAdapterMock.h"
#include "mbed-trace/mbed_trace.h"
#include <gtest/gtest.h>

#define TRACE_GROUP "ccrb-res-broker-tester"

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

    // Init resource broker ipc to be DBusAdapterMock:
    cloud_connect_resource_broker_.ipc_ = 
        std::make_unique<DBusAdapterMock>(cloud_connect_resource_broker_);
}

ResourceBrokerTester::~ResourceBrokerTester()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

void ResourceBrokerTester::add_objects(const M2MObjectList& /*object_list*/)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    // Currently does nothing, in future test we might want to add more code here
}

void ResourceBrokerTester::register_update()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    // Currently does nothing, in future test we might want to add more code here
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

void ResourceBrokerTester::mbed_client_register_update_callback_test(
    const std::string& access_token,
    CloudConnectStatus dbus_adapter_expected_status)
{
    // Call Application Endpoint to notify registration was successful
    ASSERT_TRUE(cloud_connect_resource_broker_.app_endpoints_map_.end() !=
        cloud_connect_resource_broker_.app_endpoints_map_.find(access_token));
    mbl::ResourceBroker::SPApplicationEndpoint app_endpoint = 
        cloud_connect_resource_broker_.app_endpoints_map_[access_token];

    // Make sure application is not yet registered
    ASSERT_FALSE(app_endpoint->registered_);

    if(dbus_adapter_expected_status == CloudConnectStatus::STATUS_SUCCESS) {
        // Check registration success flow
        tr_debug("%s: Notify Application (access_token: %s) that registration was successful",
            __PRETTY_FUNCTION__,
            access_token.c_str());

        // Next call will invoke resource-broker's cb function with will then call DBusAdapterTest
        app_endpoint->handle_registration_updated_cb();
        ASSERT_TRUE(app_endpoint->registered_);
    } else {
        // Check registration failure flow
        tr_debug("%s: Notify Application (access_token: %s) that registration failed",
            __PRETTY_FUNCTION__,
            access_token.c_str());

        // Next call will invoke resource-broker's cb function with will then call DBusAdapterTest
        app_endpoint->handle_error_cb(MbedCloudClient::ConnectInvalidParameters);
    }

    DBusAdapterMock& dbus_adapter_tester = 
        *(static_cast<DBusAdapterMock*>(cloud_connect_resource_broker_.ipc_.get()));
    // Verify that resource broker called the adapter (both for success and failure)
    ASSERT_TRUE(dbus_adapter_tester.is_update_registration_called());
    // Verify adapter got the right call from resource broker
    ASSERT_TRUE(dbus_adapter_expected_status == 
        dbus_adapter_tester.get_register_cloud_connect_status());
}
