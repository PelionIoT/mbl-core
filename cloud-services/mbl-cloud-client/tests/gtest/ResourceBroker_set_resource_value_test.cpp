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

#include "TestInfra.h"
#include "ResourceBrokerTester.h"
#include "ResourceDefinitionJson.h"
#include "CloudConnectTypes.h"

#include <gtest/gtest.h>

#define TRACE_GROUP "ccrb-set_res_value-test"


struct GetM2MResourceEntry{
    const std::string application_resource_definition;
    const std::string resource_path;
    mbl::MblError expected_error_status;
};

const static std::vector<GetM2MResourceEntry> get_m2m_resource_entry_vector = {
    {
        VALID_APP_RESOURCE_DEFINITION_ONE_DYNAMIC_OBJECT_WITH_ONE_OBJECT_INSTANCE_AND_TWO_RESOURCE,
        "/8888/11/111",
        mbl::Error::None
    },

    {
        VALID_APP_RESOURCE_DEFINITION_ONE_DYNAMIC_OBJECT_WITH_ONE_OBJECT_INSTANCE_AND_TWO_RESOURCE,
        "/8888/11/112",
        mbl::Error::None    
    },
    {
        VALID_APP_RESOURCE_DEFINITION_ONE_DYNAMIC_OBJECT_WITH_ONE_OBJECT_INSTANCE_AND_TWO_RESOURCE,
        "/8888", // Path should be 3 level depth
        mbl::MblError::CCRBInvalidResourcePath        
    },

    {
        VALID_APP_RESOURCE_DEFINITION_ONE_DYNAMIC_OBJECT_WITH_ONE_OBJECT_INSTANCE_AND_TWO_RESOURCE,
        "/8888/11", // Path should be 3 level depth
        mbl::MblError::CCRBInvalidResourcePath
    },

    {
        VALID_APP_RESOURCE_DEFINITION_ONE_DYNAMIC_OBJECT_WITH_ONE_OBJECT_INSTANCE_AND_TWO_RESOURCE,
        "/8888/11/111/1/2/3", // Path should be 3 level depth
        mbl::MblError::CCRBInvalidResourcePath    
    },

    {
        VALID_APP_RESOURCE_DEFINITION_ONE_DYNAMIC_OBJECT_WITH_ONE_OBJECT_INSTANCE_AND_TWO_RESOURCE,
        "/8888/11abc/111", // Object instance should be a number
        mbl::MblError::CCRBInvalidResourcePath    
    },

    {
        VALID_APP_RESOURCE_DEFINITION_ONE_DYNAMIC_OBJECT_WITH_ONE_OBJECT_INSTANCE_AND_TWO_RESOURCE,
        "/8888/11/333",
        mbl::Error::CCRBResourceNotFound    
    },

    {
        VALID_APP_RESOURCE_DEFINITION_ONE_DYNAMIC_OBJECT_WITH_ONE_OBJECT_INSTANCE_AND_TWO_RESOURCE,
        "/8888/33/111",
        mbl::Error::CCRBResourceNotFound   
    },

    {
        VALID_APP_RESOURCE_DEFINITION_ONE_DYNAMIC_OBJECT_WITH_ONE_OBJECT_INSTANCE_AND_TWO_RESOURCE,
        "/9999/11/111",
        mbl::Error::CCRBResourceNotFound   
    }
};

class ResourceBrokerTest : public testing::TestWithParam<int>{};
INSTANTIATE_TEST_CASE_P(GetM2MResourceParameterizedTest,
                        ResourceBrokerTest,
                        ::testing::Range(0, (int)get_m2m_resource_entry_vector.size(), 1));

/**
 * @brief Test valid and invalid resource paths
 * 
 * The tested scenario is:
 * 1. Resource Broker is called for register_resources() API
 * 2. Application resource definition is parsed by ApplicationEndpoint class
 * 3. Resource Broker registers ApplicationEndpoint's callbacks so Mbed client will call them
 * 4. Resource Broker calls Mbed cloud client to register the resources
 * 5. Get resource with valid path
 */

TEST_P(ResourceBrokerTest, get_m2m_resource)
{
    GTEST_LOG_START_TEST;

    const GetM2MResourceEntry &test_data = 
        get_m2m_resource_entry_vector[(size_t)GetParam()];

    ResourceBrokerTester resource_broker_tester;
    CloudConnectStatus cloud_connect_out_status;
    std::string out_access_token;

    resource_broker_tester.register_resources_test(
        mbl::IpcConnection("source1"),        
        test_data.application_resource_definition,
        cloud_connect_out_status,
        out_access_token,
        mbl::MblError::None, // expected error status
        CloudConnectStatus::STATUS_SUCCESS // expected cloud connect status
    );

    // Check valid resource path
    resource_broker_tester.get_m2m_resource_test(
        out_access_token,
        test_data.resource_path,
        test_data.expected_error_status);
}

/**
 * @brief Test setting value of string and integer
 * 
 */
TEST(Resource_Broker_Positive, set_resource_value) {

    GTEST_LOG_START_TEST;

    ResourceBrokerTester resource_broker_tester;
    const std::string application_resource_definition = 
        VALID_APP_RESOURCE_DEFINITION_ONE_DYNAMIC_OBJECT_WITH_ONE_OBJECT_INSTANCE_AND_TWO_RESOURCE;
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

    // Above application resource definition register the following resources:
    // 1. /8888/11/111 - dynamic string
    // 2. /8888/11/112 - dynamic integer

    std::vector<mbl::ResourceSetOperation> inout_set_operations;
    mbl::ResourceData input_data_1("/8888/11/111", "string_value");
    mbl::ResourceData input_data_2("/8888/11/112", 555);
    mbl::ResourceSetOperation res_set_op_1(input_data_1);
    mbl::ResourceSetOperation res_set_op_2(input_data_2);
    inout_set_operations.push_back(res_set_op_1);
    inout_set_operations.push_back(res_set_op_2);

    std::vector<mbl::ResourceSetOperation> expected_inout_set_operations = inout_set_operations;

    // Set expected statuses
    expected_inout_set_operations[0].output_status_ = CloudConnectStatus::STATUS_SUCCESS;
    expected_inout_set_operations[1].output_status_ = CloudConnectStatus::STATUS_SUCCESS;

    resource_broker_tester.set_resources_values_test(
        out_access_token,
        inout_set_operations,
        expected_inout_set_operations,
        CloudConnectStatus::STATUS_SUCCESS // expected cloud connect status
    );
}

/**
 * @brief Test setting value of string to an integer resource and vica versa
 * 
 */
TEST(Resource_Broker_Negative, set_resource_value_invalid_type) {

    GTEST_LOG_START_TEST;

    ResourceBrokerTester resource_broker_tester;
    const std::string application_resource_definition = 
        VALID_APP_RESOURCE_DEFINITION_ONE_DYNAMIC_OBJECT_WITH_ONE_OBJECT_INSTANCE_AND_TWO_RESOURCE;
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

    // Above application resource definition register the following resources:
    // 1. /8888/11/111 - dynamic string
    // 2. /8888/11/112 - dynamic integer

    std::vector<mbl::ResourceSetOperation> inout_set_operations;
    mbl::ResourceData input_data_1("/8888/11/111", 123); // should be a string
    mbl::ResourceData input_data_2("/8888/11/112", "this_is_not_an_integer"); // should be integer
    mbl::ResourceSetOperation res_set_op_1(input_data_1);
    mbl::ResourceSetOperation res_set_op_2(input_data_2);
    inout_set_operations.push_back(res_set_op_1);
    inout_set_operations.push_back(res_set_op_2);

    std::vector<mbl::ResourceSetOperation> expected_inout_set_operations = inout_set_operations;

    // Set expected statuses
    expected_inout_set_operations[0].output_status_ = CloudConnectStatus::ERR_INVALID_RESOURCE_TYPE;
    expected_inout_set_operations[1].output_status_ = CloudConnectStatus::ERR_INVALID_RESOURCE_TYPE;

    resource_broker_tester.set_resources_values_test(
        out_access_token,
        inout_set_operations,
        expected_inout_set_operations,
        CloudConnectStatus::STATUS_SUCCESS // expected cloud connect status
    );
}

/**
 * @brief Test setting value resource that wasn't registered
 * 
 */
TEST(Resource_Broker_Negative, set_resource_value_resource_not_found) {

    GTEST_LOG_START_TEST;

    ResourceBrokerTester resource_broker_tester;
    const std::string application_resource_definition = 
        VALID_APP_RESOURCE_DEFINITION_ONE_DYNAMIC_OBJECT_WITH_ONE_OBJECT_INSTANCE_AND_TWO_RESOURCE;
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

    // Above application resource definition register the following resources:
    // 1. /8888/11/111 - dynamic string
    // 2. /8888/11/112 - dynamic integer

    std::vector<mbl::ResourceSetOperation> inout_set_operations;
    mbl::ResourceData input_data_1("/9999/11/111", "string_value"); //no such resource
    mbl::ResourceData input_data_2("/8888/99/112", 555); //no such resource
    mbl::ResourceSetOperation res_set_op_1(input_data_1);
    mbl::ResourceSetOperation res_set_op_2(input_data_2);
    inout_set_operations.push_back(res_set_op_1);
    inout_set_operations.push_back(res_set_op_2);

    std::vector<mbl::ResourceSetOperation> expected_inout_set_operations = inout_set_operations;

    // Set expected statuses
    expected_inout_set_operations[0].output_status_ = CloudConnectStatus::ERR_RESOURCE_NOT_FOUND;
    expected_inout_set_operations[1].output_status_ = CloudConnectStatus::ERR_RESOURCE_NOT_FOUND;

    resource_broker_tester.set_resources_values_test(
        out_access_token,
        inout_set_operations,
        expected_inout_set_operations,
        CloudConnectStatus::STATUS_SUCCESS // expected cloud connect status
    );
}

/**
 * @brief Test setting value resource with invalid path
 * 
 */
TEST(Resource_Broker_Negative, set_resource_value_invalid_path) {

    GTEST_LOG_START_TEST;

    ResourceBrokerTester resource_broker_tester;
    const std::string application_resource_definition = 
        VALID_APP_RESOURCE_DEFINITION_ONE_DYNAMIC_OBJECT_WITH_ONE_OBJECT_INSTANCE_AND_TWO_RESOURCE;
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

    // Above application resource definition register the following resources:
    // 1. /8888/11/111 - dynamic string
    // 2. /8888/11/112 - dynamic integer

    std::vector<mbl::ResourceSetOperation> inout_set_operations;
    mbl::ResourceData input_data_1("8888/11/111", "string_value"); // path should start with "/"
    mbl::ResourceData input_data_2("/8888/11", 555); // path not 3 level depth
    mbl::ResourceSetOperation res_set_op_1(input_data_1);
    mbl::ResourceSetOperation res_set_op_2(input_data_2);
    inout_set_operations.push_back(res_set_op_1);
    inout_set_operations.push_back(res_set_op_2);

    std::vector<mbl::ResourceSetOperation> expected_inout_set_operations = inout_set_operations;

    // Set expected statuses
    expected_inout_set_operations[0].output_status_ = CloudConnectStatus::ERR_INVALID_RESOURCE_PATH;
    expected_inout_set_operations[1].output_status_ = CloudConnectStatus::ERR_INVALID_RESOURCE_PATH;

    resource_broker_tester.set_resources_values_test(
        out_access_token,
        inout_set_operations,
        expected_inout_set_operations,
        CloudConnectStatus::STATUS_SUCCESS // expected cloud connect status
    );
}

/**
 * @brief Test setting value resource with invalid access token
 * 
 */

TEST(Resource_Broker_Negative, set_resource_invalid_access_token) {

    GTEST_LOG_START_TEST;

    std::vector<mbl::ResourceSetOperation> inout_set_operations;
    mbl::ResourceData input_data_1("8888/11/111", "string_value"); // resource not registered
    mbl::ResourceSetOperation res_set_op_1(input_data_1);
    inout_set_operations.push_back(res_set_op_1);

    std::string invalid_access_token("invalid_access_token");
    std::vector<mbl::ResourceSetOperation> expected_inout_set_operations = inout_set_operations;
    ResourceBrokerTester resource_broker_tester;
    resource_broker_tester.set_resources_values_test(
        invalid_access_token,
        inout_set_operations,
        expected_inout_set_operations,
        CloudConnectStatus::ERR_INVALID_ACCESS_TOKEN // expected cloud connect status
    );
}
