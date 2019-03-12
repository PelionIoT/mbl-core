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
#include "MbedCloudClient.h"
#include "TestInfra.h"
#include "ResourceBrokerTester.h"
#include "ResourceDefinitionJson.h"

#include <gtest/gtest.h>

#define TRACE_GROUP "ccrb-register-test"

/**
 * @brief This test successful registration
 * 
 * The tested scenario is:
 * 1. Resource Broker is called for register_resources() API
 * 2. Application resource definition is parsed by ApplicationEndpoint class
 * 3. Resource Broker registers ApplicationEndpoint's callbacks so Mbed client will call them
 * 4. Resource Broker calls Mbed cloud client to register the resources
 * 5. Mbed cloud client calls ApplicationEndpoint's REGISTER callback upon SUCCESSFUL registration
 * 6. ApplicationEndpoint notify the Resource Broker that registration was SUCCESSFUL
 * 7. Resource Broker notifies DbusAdapter that registration was SUCCESSFUL
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
        mbl::MblError::None, // expected error status
        CloudConnectStatus::STATUS_SUCCESS // expected cloud connect status
    );

    // Test registration callback succeeded
    resource_broker_tester.mbed_client_register_update_callback_test(
        out_access_token,
        CloudConnectStatus::STATUS_SUCCESS);
}

/**
 * @brief This test failed registration
 * 
 * The tested scenario is:
 * 1. Resource Broker is called for register_resources() API
 * 2. Application resource definition is parsed by ApplicationEndpoint class
 * 3. Resource Broker registers ApplicationEndpoint's callbacks so Mbed client will call them
 * 4. Resource Broker calls Mbed cloud client to register the resources
 * 5. Mbed cloud client calls ApplicationEndpoint's ERROR callbacks upon FAILED registration
 * 6. ApplicationEndpoint notify the Resource Broker that registration FAILED
 * 7. Resource Broker notifys DbusAdapter that registration was FAILED
 */
TEST(Resource_Broker_Negative, parsing_succedded_registration_failed)
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
        mbl::MblError::None, //expected error status
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
 * 2. Application resource definition is parsed by ApplicationEndpoint class but FAIL due to invalid JSON
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
        mbl::MblError::None, //expected error status
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
        mbl::MblError::None, //expected error status
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
        mbl::MblError::None, //expected error status
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
        mbl::MblError::None, //expected error status
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
        mbl::MblError::None, //expected error status
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
        mbl::MblError::None, //expected error status
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
        mbl::MblError::None, //expected error status
        CloudConnectStatus::STATUS_SUCCESS //expected cloud connect status
    );

    // Test registration callback failure
    resource_broker_tester.mbed_client_register_update_callback_test(
        out_access_token,
        CloudConnectStatus::STATUS_SUCCESS);    
}

void ResourceBroker_start_stop(size_t times) {

    mbl::ResourceBroker resource_broker;
    for( size_t i = 0; i < times; ++i){
        ASSERT_EQ(mbl::Error::None, resource_broker.start(nullptr));
        ASSERT_EQ(mbl::Error::None, resource_broker.stop());
    }
}

TEST(Resource_Broker_Positive, start_stop_20_times) {

    GTEST_LOG_START_TEST;
    ResourceBroker_start_stop(20);
}

TEST(Resource_Broker_Positive, start_stop) {

    GTEST_LOG_START_TEST;
    ResourceBroker_start_stop(1);
}