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
#include "CloudConnectTrace.h"
#include "CloudConnectTypes.h"

#include <gtest/gtest.h>

#define TRACE_GROUP "ccrb-res-broker-tester"

ResourceBrokerTester::ResourceBrokerTester()
{
    TR_DEBUG_ENTER;

    // Set Resource Broker function pointers to point to this class instead of to Mbed Client
    // This replaces what resource_broker_.init() usually does so dont call it...
    resource_broker_.register_update_func_ = 
        std::bind(&ResourceBrokerTester::register_update, this);
    resource_broker_.add_objects_func_ = 
        std::bind(static_cast<void(ResourceBrokerTester::*)(const M2MObjectList&)>(&ResourceBrokerTester::add_objects),
            this,
            std::placeholders::_1);

    // Init resource broker ipc to be DBusAdapterMock:
    resource_broker_.ipc_adapter_ = 
        std::make_unique<DBusAdapterMock>(resource_broker_);
}

ResourceBrokerTester::~ResourceBrokerTester()
{
    TR_DEBUG_ENTER;
}

void ResourceBrokerTester::add_objects(const M2MObjectList& /*object_list*/)
{
    TR_DEBUG_ENTER;
    // Currently does nothing, in future test we might want to add more code here
}

void ResourceBrokerTester::register_update()
{
    TR_DEBUG_ENTER;
    // Currently does nothing, in future test we might want to add more code here
}

void
ResourceBrokerTester::register_resources_test(
    const mbl::IpcConnection &source,
    const std::string& app_resource_definition,
    CloudConnectStatus& out_status,
    std::string& out_access_token,
    CloudConnectStatus expected_cloud_connect_status)
{
    TR_DEBUG_ENTER;

    std::pair<CloudConnectStatus, std::string> ret_pair = 
        resource_broker_.register_resources(source, app_resource_definition);

    // Check cloud connect expected status
    ASSERT_TRUE(expected_cloud_connect_status == ret_pair.first);
    out_status = ret_pair.first;
    out_access_token = ret_pair.second;
}

void ResourceBrokerTester::mbed_client_register_update_callback_test(
    const std::string& access_token,
    CloudConnectStatus dbus_adapter_expected_status)
{
    TR_DEBUG_ENTER;

    auto itr = resource_broker_.registration_records_.find(access_token);
    ASSERT_TRUE(resource_broker_.registration_records_.end() !=
        itr);
    mbl::ResourceBroker::RegistrationRecord_ptr registration_record = itr->second;

    // Make sure registration_record is not yet registered
    ASSERT_FALSE(registration_record->is_registered());

    if(dbus_adapter_expected_status == CloudConnectStatus::STATUS_SUCCESS) {
        // Check registration success flow
        TR_DEBUG("Notify resource broker (access_token: %s) that registration was successful",
            access_token.c_str());
        resource_broker_.handle_registration_updated_cb();
        
        // Make sure registration record is marked as registered
        ASSERT_TRUE(registration_record->is_registered());
    } else {
        // Check registration failure flow
        TR_DEBUG("Notify resource broker (access_token: %s) that registration failed",
            access_token.c_str());
        // Next call will invoke resource-broker's cb function with will then call DBusAdapterTest
        resource_broker_.handle_error_cb(MbedCloudClient::ConnectInvalidParameters);
    }

    DBusAdapterMock& dbus_adapter_tester = 
        *(static_cast<DBusAdapterMock*>(resource_broker_.ipc_adapter_.get()));
    // Verify that resource broker called the adapter (both for success and failure)
    ASSERT_TRUE(dbus_adapter_tester.is_update_registration_called());
    // Verify adapter got the right call from resource broker
    ASSERT_TRUE(dbus_adapter_expected_status == 
        dbus_adapter_tester.get_register_cloud_connect_status());
}

void ResourceBrokerTester::get_m2m_resource_test(
    const std::string& access_token,
    const std::string& path,
    mbl::MblError expected_error_status)
{
    TR_DEBUG_ENTER;

    auto itr = resource_broker_.registration_records_.find(access_token);
    ASSERT_TRUE(resource_broker_.registration_records_.end() !=
        itr);
    mbl::ResourceBroker::RegistrationRecord_ptr registration_record = itr->second;

    std::pair<mbl::MblError, M2MResource*> ret_pair = registration_record->get_m2m_resource(path);

    ASSERT_TRUE(expected_error_status == ret_pair.first);

    // In case of success - verify valid resource
    if (mbl::Error::None == expected_error_status) {
        ASSERT_TRUE(nullptr != ret_pair.second);
    }
}

void ResourceBrokerTester::set_resources_values_test(
    const std::string& access_token,
    std::vector<mbl::ResourceSetOperation>& inout_set_operations,
    const std::vector<mbl::ResourceSetOperation> expected_inout_set_operations,
    const CloudConnectStatus expected_out_status)
{
    TR_DEBUG_ENTER;
    
    CloudConnectStatus out_status = 
        resource_broker_.set_resources_values(
            mbl::IpcConnection("source1"),
            access_token,
            inout_set_operations);
    
    ASSERT_TRUE(expected_out_status == out_status);

    if(CloudConnectStatus::STATUS_SUCCESS != out_status) {
        return; // Nothing left to check
    }

    // Note: both vectors should be with the same size and items order or test will fail.
    ASSERT_TRUE(expected_inout_set_operations.size() == inout_set_operations.size());
    
    // Compare expected_out_status to out_status
    auto expected_itr = expected_inout_set_operations.begin();
    for (auto in_out_itr : inout_set_operations) {
        std::string path = in_out_itr.input_data_.get_path();
        ASSERT_TRUE(path == expected_itr->input_data_.get_path());
        ASSERT_TRUE(expected_itr->output_status_ == in_out_itr.output_status_);
        std::advance(expected_itr, 1); // Move to next item
    }
}

void ResourceBrokerTester::get_resources_values_test(
    const std::string& access_token,
    std::vector<mbl::ResourceGetOperation>& inout_get_operations,
    const std::vector<mbl::ResourceGetOperation> expected_inout_get_operations,
    const CloudConnectStatus expected_out_status)
{
    TR_DEBUG("Enter");
    
    CloudConnectStatus out_status =
        resource_broker_.get_resources_values(mbl::IpcConnection("source1"),
                                              access_token,
                                              inout_get_operations);
    
    ASSERT_TRUE(expected_out_status == out_status);

    if(CloudConnectStatus::STATUS_SUCCESS != out_status) {
        return; // Nothing left to check
    }

    // Note: both vectors should be with the same size and items order or test will fail.
    ASSERT_TRUE(expected_inout_get_operations.size() == inout_get_operations.size());
    
    // Compare expected_out_status to out_status
    auto expected_itr = expected_inout_get_operations.begin();
    for (auto in_out_itr : inout_get_operations) {
        std::string path = in_out_itr.inout_data_.get_path();
        ASSERT_TRUE(path == expected_itr->inout_data_.get_path());

        ASSERT_TRUE(expected_itr->output_status_ == in_out_itr.output_status_);
        if(CloudConnectStatus::STATUS_SUCCESS != expected_itr->output_status_) {
            std::advance(expected_itr, 1); // Move to next item
            continue; // Expected failed to get this resource value, continue...
        }

        ResourceDataType expected_data_type = expected_itr->inout_data_.get_data_type();

        ASSERT_TRUE(expected_data_type == in_out_itr.inout_data_.get_data_type());
        switch(expected_data_type) 
        {
        case ResourceDataType::STRING:
        {
            std::string expected_value_string = 
                expected_itr->inout_data_.get_value_string();
            ASSERT_TRUE(expected_value_string 
                == in_out_itr.inout_data_.get_value_string());
            break;
        }
        case ResourceDataType::INTEGER:
        {
            int64_t expected_value_integer = 
                expected_itr->inout_data_.get_value_integer();
            ASSERT_TRUE(expected_value_integer 
                == in_out_itr.inout_data_.get_value_integer());
            break;
        }
        default:
        {
            ASSERT_TRUE(false); // Always fail as we only support integer and strings
            break;
        }
        }
        std::advance(expected_itr, 1); // Move to next item
    }
}

void ResourceBrokerTester::notify_connection_closed_test_multiple_reg_records()
{
    mbl::IpcConnection source_1("source1"), source_2("source2"), source_3("source3");
    mbl::ResourceBroker::RegistrationRecord_ptr registration_record_1 = 
        std::make_shared<mbl::RegistrationRecord>(source_1);
    mbl::ResourceBroker::RegistrationRecord_ptr registration_record_2 = 
        std::make_shared<mbl::RegistrationRecord>(source_2);
    mbl::ResourceBroker::RegistrationRecord_ptr registration_record_3 = 
        std::make_shared<mbl::RegistrationRecord>(source_3);
    // Add registration records to map
    resource_broker_.registration_records_["registration_record_1"] = registration_record_1;
    resource_broker_.registration_records_["registration_record_2"] = registration_record_2;
    resource_broker_.registration_records_["registration_record_3"] = registration_record_3;

    // Add source 1 and source 2 to both registration_records
    registration_record_1->track_ipc_connection(source_2,
        mbl::RegistrationRecord::TrackOperation::ADD); // Now track source_1 and 2
    registration_record_2->track_ipc_connection(source_1,
        mbl::RegistrationRecord::TrackOperation::ADD); // Now track source_1 and 2

    // Verify we have 3 registration records
    ASSERT_TRUE(resource_broker_.registration_records_.size() == 3);

    // Verify we not have 2 registration records as registration record 3 is erased
    resource_broker_.notify_connection_closed(source_3);
    ASSERT_TRUE(resource_broker_.registration_records_.size() == 2);

    // Mark source_1 resource as closed
    resource_broker_.notify_connection_closed(source_1);

    // Verify number of registration records is still 2
    ASSERT_TRUE(resource_broker_.registration_records_.size() == 2);

    // Mark source_1 resource as closed
    resource_broker_.notify_connection_closed(source_2);

    ASSERT_TRUE(resource_broker_.registration_records_.empty());
}

void ResourceBrokerTester::notify_connection_closed(mbl::IpcConnection source) 
{
    resource_broker_.notify_connection_closed(source);
}