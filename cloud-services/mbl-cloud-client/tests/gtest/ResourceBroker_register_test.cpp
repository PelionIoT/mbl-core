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
#include "CloudConnectTypes.h"
#include "ResourceBroker.h"
#include "mbed-cloud-client/MbedCloudClient.h"
#include "TestInfra.h"
#include "ResourceBrokerTester.h"
#include "ResourceDefinitionJson.h"

#include <gtest/gtest.h>

#define TRACE_GROUP "ccrb-register-test"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Positive tests
////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief This test successful registration
 * 
 * The tested scenario is:
 * 1. Resource Broker is called for register_resources() API
 * 2. Application resource definition is parsed by RegistrationRecord class
 * 3. Resource Broker calls Mbed cloud client to register the resources
 * 4. Mbed cloud client calls ccrb REGISTER callback upon SUCCESSFUL registration
 * 5. Resource Broker notifies DbusAdapter that registration was SUCCESSFUL
 */
TEST(Resource_Broker_Positive, registration_success) 
{
    GTEST_LOG_START_TEST;

    ResourceBrokerTester resource_broker_tester;
    const std::string application_resource_definition = 
        VALID_APP_RESOURCE_DEFINITION_TWO_OBJECTS_WITH_ONE_OBJECT_INSTANCE_AND_ONE_RESOURCE;
    CloudConnectStatus cloud_connect_out_status;
    std::string out_access_token;

    resource_broker_tester.register_resources_test(
        mbl::IpcConnection("source1"),
        application_resource_definition,
        cloud_connect_out_status,
        out_access_token,
        CloudConnectStatus::STATUS_SUCCESS // expected cloud connect status
    );

    // Test registration callback succeeded
    resource_broker_tester.mbed_client_register_update_callback_test(
        out_access_token,
        CloudConnectStatus::STATUS_SUCCESS);
}

/**
 * @brief This test successful registration with Mbed client callback simulation
 * This test includes Resource broker, adapter, mailbox and mbed client simulation.
 * The tested scenario is:
 * 1. Start resource broker main thread
 * 2. Resource Broker is called for register_resources() API
 * 3. Application resource definition is parsed by RegistrationRecord class
 * 4. Resource Broker calls Mbed cloud client to register the resources
 * 5. Test simulate Mbed cloud client callback for successful registration
 * 6. Resource broker send register update message to mailbox
 * 7. Resource broker is called to process register message from the adapter
 * 8. Resource Broker update registration record state to "registered"
 * 9. Stop resource broker main thread
 */
TEST(Resource_Broker_Positive, advanced_registration_success) 
{
    GTEST_LOG_START_TEST;

    ResourceBrokerTester resource_broker_tester(false);
    const std::string application_resource_definition = 
        VALID_APP_RESOURCE_DEFINITION_TWO_OBJECTS_WITH_ONE_OBJECT_INSTANCE_AND_ONE_RESOURCE;
    CloudConnectStatus cloud_connect_out_status;
    std::string out_access_token;

    resource_broker_tester.start_ccrb(); // Will fail test is start fails

    resource_broker_tester.register_resources_test(
        mbl::IpcConnection("source1"),
        application_resource_definition,
        cloud_connect_out_status,
        out_access_token,
        CloudConnectStatus::STATUS_SUCCESS // expected cloud connect status
    );

    // Test registration callback succeeded
    resource_broker_tester.simulate_mbed_client_register_update_callback_test(
        out_access_token,
        true); // true = simulatate successful registration

    resource_broker_tester.stop_ccrb(); // Will fail test is stop fails
}

/**
 * @brief Test that when registered application close its connection - it can be registered again
 * 
 * The tested scenario is:
 * 1. Resource Broker is called for register_resources() API
 * 2. Application resource definition is parsed by RegistrationRecord class
 * 3. Resource Broker calls Mbed cloud client to register the resources
 * 4. Mbed cloud client calls ccrb REGISTER callback upon SUCCESSFUL registration
 * 5. Resource Broker notifies DbusAdapter that registration was SUCCESSFUL
 * 6. Application closes its connection
 * 7. Application is able to register again (steps 1-5 are verified again)
 */
TEST(Resource_Broker_Positive, registration_success_after_connection_close) 
{
    GTEST_LOG_START_TEST;

    ResourceBrokerTester resource_broker_tester;
    const std::string application_resource_definition = 
        VALID_APP_RESOURCE_DEFINITION_TWO_OBJECTS_WITH_ONE_OBJECT_INSTANCE_AND_ONE_RESOURCE;
    CloudConnectStatus cloud_connect_out_status;
    mbl::IpcConnection ipc_connection_1("source1");

    std::string out_access_token_1;
    resource_broker_tester.register_resources_test(
        ipc_connection_1,
        application_resource_definition,
        cloud_connect_out_status,
        out_access_token_1,
        CloudConnectStatus::STATUS_SUCCESS // expected cloud connect status
    );

    // Test registration callback succeeded
    resource_broker_tester.mbed_client_register_update_callback_test(
        out_access_token_1,
        CloudConnectStatus::STATUS_SUCCESS);

    resource_broker_tester.notify_connection_closed(ipc_connection_1);

    std::string out_access_token_2;
    mbl::IpcConnection ipc_connection_2("source2");
    resource_broker_tester.register_resources_test(
        ipc_connection_2,
        application_resource_definition,
        cloud_connect_out_status,
        out_access_token_2,
        CloudConnectStatus::STATUS_SUCCESS // expected cloud connect status
    );

    // Test registration callback succeeded
    resource_broker_tester.mbed_client_register_update_callback_test(
        out_access_token_2,
        CloudConnectStatus::STATUS_SUCCESS);
}

TEST(Resource_Broker_Positive, start_stop_20_times) {

    GTEST_LOG_START_TEST;
    ResourceBrokerTester resource_broker_tester(false); // false = don't use mock dbus adapter
    resource_broker_tester.resourceBroker_start_stop_test(20);
}

TEST(Resource_Broker_Positive, start_stop) {

    GTEST_LOG_START_TEST;
    ResourceBrokerTester resource_broker_tester(false); // false = don't use mock dbus adapter
    resource_broker_tester.resourceBroker_start_stop_test(1);
}

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
TEST(Resource_Broker_Positive, notify_connection_closed_multiple_reg_records_3_connections) 
{
    GTEST_LOG_START_TEST;
    ResourceBrokerTester resource_broker_tester;
    resource_broker_tester.notify_connection_closed_test_multiple_reg_records();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Negative tests
////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief This test failed registration
 * 
 * The tested scenario is:
 * 1. Resource Broker is called for register_resources() API
 * 2. Application resource definition is parsed by RegistrationRecord class
 * 3. Resource Broker calls Mbed cloud client to register the resources
 * 4. Mbed cloud client calls ccrb ERROR callbacks upon FAILED registration
 * 5. Resource Broker notify DbusAdapter that registration was FAILED
 */
TEST(Resource_Broker_Negative, parsing_succeeded_registration_failed)
{
    GTEST_LOG_START_TEST;
    
    ResourceBrokerTester resource_broker_tester;
    const std::string application_resource_definition =
        VALID_APP_RESOURCE_DEFINITION_TWO_OBJECTS_WITH_ONE_OBJECT_INSTANCE_AND_ONE_RESOURCE;
    CloudConnectStatus cloud_connect_out_status;
    std::string out_access_token;

    resource_broker_tester.register_resources_test(
        mbl::IpcConnection("source1"),
        application_resource_definition,
        cloud_connect_out_status,
        out_access_token,
        CloudConnectStatus::STATUS_SUCCESS //expected cloud connect status
    );

    // Test registration callback failure
    resource_broker_tester.mbed_client_register_update_callback_test(
        out_access_token,
        CloudConnectStatus::ERR_INTERNAL_ERROR);
}

/**
 * @brief This test failed registration
 * 
 * The tested scenario is:
 * 1. Resource Broker is called for register_resources() API
 * 2. Application resource definition is parsed by RegistrationRecord class but FAIL due to invalid JSON
 * 3. Resource Broker return the FAILED registration cloud client status to the adapter
 */
TEST(Resource_Broker_Negative, invalid_app_resource_definition_1) 
{
    GTEST_LOG_START_TEST;

    ResourceBrokerTester resource_broker_tester;
    const std::string application_resource_definition = INVALID_APP_RESOURCE_DEFINITION_NOT_3_LEVEL_1;
    CloudConnectStatus cloud_connect_out_status;
    std::string out_access_token;

    resource_broker_tester.register_resources_test(
        mbl::IpcConnection("source1"),
        application_resource_definition,
        cloud_connect_out_status,
        out_access_token,
        CloudConnectStatus::ERR_INVALID_APPLICATION_RESOURCES_DEFINITION //expected cloud connect status
    );
}

//NOTE: this test is valid only for Single app support
TEST(Resource_Broker_Negative, already_registered) 
{
    GTEST_LOG_START_TEST;

    ResourceBrokerTester resource_broker_tester;
    const mbl::IpcConnection source1(":1.1");
    const std::string application_resource_definition_1 =
        VALID_APP_RESOURCE_DEFINITION_OBJECT_WITH_SEVERAL_OBJECT_INSTANCES_AND_RESOURCES;
    CloudConnectStatus cloud_connect_out_status;
    std::string out_access_token_1;

    // Application 1 registering
    resource_broker_tester.register_resources_test(
        source1,
        application_resource_definition_1,
        cloud_connect_out_status,
        out_access_token_1,
        CloudConnectStatus::STATUS_SUCCESS //expected cloud connect status
    );

    // Test registration callback succeeded for application 1
    resource_broker_tester.mbed_client_register_update_callback_test(
        out_access_token_1,
        CloudConnectStatus::STATUS_SUCCESS);

    // Application 2 try to register - expect fail as only one application is allowed to be registered
    const mbl::IpcConnection source2(":1.2");
    const std::string application_resource_definition_2 =
        VALID_APP_RESOURCE_DEFINITION_TWO_OBJECTS_WITH_ONE_OBJECT_INSTANCE_AND_ONE_RESOURCE;
    std::string out_access_token_2;

    resource_broker_tester.register_resources_test(
        source2,
        application_resource_definition_2,
        cloud_connect_out_status,
        out_access_token_2,
        CloudConnectStatus::ERR_ALREADY_REGISTERED //expected cloud connect status
    );    
}

TEST(Resource_Broker_Negative, registration_in_progress)
{
    GTEST_LOG_START_TEST;

    ResourceBrokerTester resource_broker_tester;
    const mbl::IpcConnection source1(":1.1");
    const std::string application_resource_definition_1 =
        VALID_APP_RESOURCE_DEFINITION_OBJECT_WITH_SEVERAL_OBJECT_INSTANCES_AND_RESOURCES;
    CloudConnectStatus cloud_connect_out_status;
    std::string out_access_token_1;

    TR_DEBUG("Application 1 - Start registration");
    resource_broker_tester.register_resources_test(
        source1,
        application_resource_definition_1,
        cloud_connect_out_status,
        out_access_token_1,
        CloudConnectStatus::STATUS_SUCCESS //expected cloud connect status
    );

    // Application 2 try to register while Application 1 registration is still in progress - expect fail
    TR_DEBUG("Application 2 - Start registration");
    const mbl::IpcConnection source2(":1.2");
    const std::string application_resource_definition_2 =
        VALID_APP_RESOURCE_DEFINITION_TWO_OBJECTS_WITH_ONE_OBJECT_INSTANCE_AND_ONE_RESOURCE;
    std::string out_access_token_2;

    resource_broker_tester.register_resources_test(
        source2,
        application_resource_definition_2,
        cloud_connect_out_status,
        out_access_token_2,
        CloudConnectStatus::ERR_REGISTRATION_ALREADY_IN_PROGRESS //expected cloud connect status
    );

    // Test registration callback succeeded for application 1
    TR_DEBUG("Application 1 - Finish registration");
    resource_broker_tester.mbed_client_register_update_callback_test(
        out_access_token_1,
        CloudConnectStatus::STATUS_SUCCESS);
}

// Test a scenario when application failed with its registration (e.g. due to communication problem)
// and succeeds in second try
TEST(Resource_Broker_Negative, first_registration_fail_second_succeeded)
{
    GTEST_LOG_START_TEST;

    ResourceBrokerTester resource_broker_tester;
    const mbl::IpcConnection source("");;
    const std::string application_resource_definition =
        VALID_APP_RESOURCE_DEFINITION_TWO_OBJECTS_WITH_ONE_OBJECT_INSTANCE_AND_ONE_RESOURCE;
    CloudConnectStatus cloud_connect_out_status;
    std::string out_access_token;

    resource_broker_tester.register_resources_test(
        source,
        application_resource_definition,
        cloud_connect_out_status,
        out_access_token,
        CloudConnectStatus::STATUS_SUCCESS //expected cloud connect status
    );

    // Test registration callback failure
    resource_broker_tester.mbed_client_register_update_callback_test(
        out_access_token,
        CloudConnectStatus::ERR_INTERNAL_ERROR);

    // Second time - this time we simulate success
    resource_broker_tester.register_resources_test(
        source,
        application_resource_definition,
        cloud_connect_out_status,
        out_access_token,
        CloudConnectStatus::STATUS_SUCCESS //expected cloud connect status
    );

    // Test registration callback failure
    resource_broker_tester.mbed_client_register_update_callback_test(
        out_access_token,
        CloudConnectStatus::STATUS_SUCCESS);    
}

/**
 * @brief This test successful registration with Mbed client callback simulation
 * This test includes Resource broker, adapter, mailbox and mbed client simulation.
 * The tested scenario is:
 * 1. Start resource broker main thread
 * 2. Resource Broker is called for register_resources() API
 * 3. Application resource definition is parsed by RegistrationRecord class
 * 4. Resource Broker calls Mbed cloud client to register the resources
 * 5. Test simulate Mbed cloud client callback for registration failure
 * 6. Resource broker send mbed client error message to mailbox
 * 7. Resource broker is called to process error message from the adapter
 * 8. Resource Broker delete registration record
 * 9. Stop resource broker main thread
 */
TEST(Resource_Broker_Negative, advanced_registration_failure) 
{
    GTEST_LOG_START_TEST;

    ResourceBrokerTester resource_broker_tester(false);
    const std::string application_resource_definition = 
        VALID_APP_RESOURCE_DEFINITION_TWO_OBJECTS_WITH_ONE_OBJECT_INSTANCE_AND_ONE_RESOURCE;
    CloudConnectStatus cloud_connect_out_status;
    std::string out_access_token;

    resource_broker_tester.start_ccrb(); // Will fail test is start fails

    resource_broker_tester.register_resources_test(
        mbl::IpcConnection("source1"),
        application_resource_definition,
        cloud_connect_out_status,
        out_access_token,
        CloudConnectStatus::STATUS_SUCCESS // expected cloud connect status
    );

    // Test registration callback succeeded
    resource_broker_tester.simulate_mbed_client_register_update_callback_test(
        out_access_token,
        false); // false = simulatate failed registration

    resource_broker_tester.stop_ccrb(); // Will fail test is stop fails
}

