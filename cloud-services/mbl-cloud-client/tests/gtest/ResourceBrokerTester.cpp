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
#include "MbedClientManager.h"
#include "CloudConnectTrace.h"
#include "CloudConnectTypes.h"
#include "MbedClientManager.h"
#include "MailboxMsg.h"
#include <gtest/gtest.h>

#define TRACE_GROUP "ccrb-res-broker-tester"

ResourceBrokerTester::ResourceBrokerTester(bool use_mock_dbus_adapter)
{
    TR_DEBUG_ENTER;

    // Set CCRB's mbed client manager to use mock (erases its member automatically)
    mbl::MbedClientManager * orig_mbed_client_manager = ccrb_.mbed_client_manager_.release();
    assert(orig_mbed_client_manager);
    delete orig_mbed_client_manager;

    ccrb_.mbed_client_manager_ = std::make_unique<MbedClientManagerMock>();

    if(use_mock_dbus_adapter) {
        // Init resource broker ipc to be DBusAdapterMock:
        ccrb_.ipc_adapter_ = 
            std::make_unique<DBusAdapterMock>(ccrb_);
    }    
}

ResourceBrokerTester::~ResourceBrokerTester()
{
    TR_DEBUG_ENTER;
}

void* ResourceBrokerTester::resource_broker_main_thread_func(void* tester)
{
    TR_DEBUG_ENTER;

    ResourceBrokerTester* this_tester = static_cast<ResourceBrokerTester*>(tester);

    this_tester->ccrb_.run(); // CHECK RETURN VALUE//////////////////////////////////////////////////////////////
    pthread_exit((void*) 0);
}

pthread_t ResourceBrokerTester::start_ccrb() ///////////////////////////////////////////////////////////////////RENAME TO RUN
{
    pthread_t resource_broker_main_thread_id = 0;
    pthread_create(&resource_broker_main_thread_id,
                    nullptr, // thread is created with default attributes
                    ResourceBrokerTester::resource_broker_main_thread_func,
                    this);
    // Wait until mbed client is registered to mark when init is done
    for(;;) {
        if (mbl::MbedClientManager::State_DeviceRegistered ==
                ccrb_.mbed_client_manager_->get_device_state())
        {
            break;
        }
        sleep(1);
    }
    return resource_broker_main_thread_id;    
}

void ResourceBrokerTester::stop_ccrb(pthread_t resource_broker_main_thread_id)
{
    kill(getpid(),SIGUSR1); ///check ret val /////////////////////////////////////////////////////////////////
    void* retval;
    ASSERT_EQ(pthread_join(resource_broker_main_thread_id, &retval), 0);
}

void ResourceBrokerTester::register_resources_test(
    const mbl::IpcConnection &source,
    const std::string& app_resource_definition,
    CloudConnectStatus& out_status,
    std::string& out_access_token,
    CloudConnectStatus expected_cloud_connect_status)
{
    TR_DEBUG_ENTER;

    std::pair<CloudConnectStatus, std::string> ret_pair = 
        ccrb_.register_resources(source, app_resource_definition);

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

    auto itr = ccrb_.registration_records_.find(access_token);
    ASSERT_TRUE(ccrb_.registration_records_.end() != itr);
    mbl::ResourceBroker::RegistrationRecord_ptr registration_record = itr->second;

    // Make sure registration_record state is register in progress
    ASSERT_TRUE(mbl::RegistrationRecord::State_RegistrationInProgress == 
        registration_record->get_registration_state());

    if(is_CloudConnectStatus_not_error(dbus_adapter_expected_status)) {
        // Check registration success flow
        TR_DEBUG("Notify resource broker (access_token: %s) that registration was successful",
            access_token.c_str());

        // Next calls doesn't check sending and receiving of mailbox messages as this is tested elsewhere
        ccrb_.handle_resources_registration_succeeded_internal_message();
        
        // Make sure registration record is marked as registered
        ASSERT_TRUE(mbl::RegistrationRecord::State_Registered == 
            registration_record->get_registration_state());

    } else {

        // Check registration failure flow
        TR_DEBUG("Notify resource broker (access_token: %s) that registration failed",
            access_token.c_str());
        
        // Next calls doesn't check sending and receiving of mailbox messages as this is tested elsewhere
        ccrb_.handle_mbed_client_error_internal_message(
            mbl::MblError::ConnectInvalidParameters);  // This error require action!
    }

    DBusAdapterMock& dbus_adapter_tester = 
        *(static_cast<DBusAdapterMock*>(ccrb_.ipc_adapter_.get()));
    // Verify that resource broker called the adapter (both for success and failure)
    ASSERT_TRUE(dbus_adapter_tester.is_update_registration_called());
    // Verify adapter got the right call from resource broker
    ASSERT_TRUE(dbus_adapter_expected_status == 
        dbus_adapter_tester.get_register_cloud_connect_status());
}

struct RegistrationData {
    ResourceBrokerTester* tester;
    bool simulate_registration_success;
};

void* ResourceBrokerTester::mbed_client_mock_thread_func(void* data)
{
    TR_DEBUG_ENTER;
    RegistrationData* registration_data = static_cast<RegistrationData*>(data);

    if(registration_data->simulate_registration_success) {
        registration_data->tester->ccrb_.resources_registration_succeeded();
    } else {
        registration_data->tester->ccrb_.handle_mbed_client_error(
            mbl::MblError::Unknown);
    }
    sleep(1); // Allow mailbox to call resource broker to handle above messages
    pthread_exit((void*) 0);
}

void ResourceBrokerTester::simulate_mbed_client_register_update_callback_test(
    const std::string& access_token,
    bool simulate_registration_success)
{
    TR_DEBUG_ENTER;

    auto itr = ccrb_.registration_records_.find(access_token);
    ASSERT_TRUE(ccrb_.registration_records_.end() != itr);
    mbl::ResourceBroker::RegistrationRecord_ptr registration_record = itr->second;

    // Make sure registration_record state is register in progress
    ASSERT_TRUE(mbl::RegistrationRecord::State_RegistrationInProgress == 
        registration_record->get_registration_state());

    // Create new thread that will simulate mbed client callback (registration failed / succeeded)
    struct RegistrationData registration_data{this, simulate_registration_success};
    pthread_t mbed_client_thread_id = 0;
    ASSERT_EQ(pthread_create(&mbed_client_thread_id,
                         nullptr, // thread is created with default attributes
                         ResourceBrokerTester::mbed_client_mock_thread_func,
                         &registration_data), 0);
    void* retval;
    ASSERT_EQ(pthread_join(mbed_client_thread_id, &retval), 0);

    if(simulate_registration_success) {
        // Make sure registration_record state is register in progress
        // (we can still use above registration record shared ptr)
        ASSERT_TRUE(mbl::RegistrationRecord::State_Registered == 
            registration_record->get_registration_state());
    } else
    {
        // Make sure registration record was erased from map as it is failed registration
        itr = ccrb_.registration_records_.find(access_token);
        ASSERT_TRUE(ccrb_.registration_records_.end() == itr); // Does not exist!
    }
    
}


void ResourceBrokerTester::get_m2m_resource_test(
    const std::string& access_token,
    const std::string& path,
    mbl::MblError expected_error_status)
{
    TR_DEBUG_ENTER;

    auto itr = ccrb_.registration_records_.find(access_token);
    ASSERT_TRUE(ccrb_.registration_records_.end() !=
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
        ccrb_.set_resources_values(
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
    TR_DEBUG_ENTER;
    
    CloudConnectStatus out_status =
        ccrb_.get_resources_values(
            mbl::IpcConnection("source1"),
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

void ResourceBrokerTester::resourceBroker_start_stop_test(size_t times)
{
     for( size_t i = 0; i < times; ++i){
        // ASSERT_EQ(mbl::Error::None, ccrb_.start());
        // ASSERT_EQ(mbl::Error::None, ccrb_.stop());
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
    ccrb_.registration_records_["registration_record_1"] = registration_record_1;
    ccrb_.registration_records_["registration_record_2"] = registration_record_2;
    ccrb_.registration_records_["registration_record_3"] = registration_record_3;

    // Add source 1 and source 2 to both registration_records
    registration_record_1->track_ipc_connection(source_2,
        mbl::RegistrationRecord::TrackOperation::ADD); // Now track source_1 and 2
    registration_record_2->track_ipc_connection(source_1,
        mbl::RegistrationRecord::TrackOperation::ADD); // Now track source_1 and 2

    // Verify we have 3 registration records
    ASSERT_TRUE(ccrb_.registration_records_.size() == 3);

    // Verify we not have 2 registration records as registration record 3 is erased
    ccrb_.notify_connection_closed(source_3);
    ASSERT_TRUE(ccrb_.registration_records_.size() == 2);

    // Mark source_1 resource as closed
    ccrb_.notify_connection_closed(source_1);

    // Verify number of registration records is still 2
    ASSERT_TRUE(ccrb_.registration_records_.size() == 2);

    // Mark source_1 resource as closed
    ccrb_.notify_connection_closed(source_2);

    ASSERT_TRUE(ccrb_.registration_records_.empty());
}

void ResourceBrokerTester::notify_connection_closed(mbl::IpcConnection source) 
{
    ccrb_.notify_connection_closed(source);
}
