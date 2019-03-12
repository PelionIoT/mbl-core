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

////////////////////////////////////////////////////////////////////////////////////////////////////
// get_m2m_resource tests
////////////////////////////////////////////////////////////////////////////////////////////////////

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

class ResourceBrokerGetM2MResourceTest : public testing::TestWithParam<int>{};
INSTANTIATE_TEST_CASE_P(GetM2MResourceParameterizedTest,
                        ResourceBrokerGetM2MResourceTest,
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
TEST_P(ResourceBrokerGetM2MResourceTest, get_m2m_resource)
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

////////////////////////////////////////////////////////////////////////////////////////////////////
// Set resource value tests
////////////////////////////////////////////////////////////////////////////////////////////////////

        ipc_conn_handle,
struct SetResourcesValueEntry{
     // Register resources
    const std::string application_resource_definition;
    // Set resource data
    mbl::ResourceData set_resource_data_1;
    mbl::ResourceData set_resource_data_2;
    CloudConnectStatus expected_set_resource_status;
    // Expected set resource data
    CloudConnectStatus expected_get_resource_value_status_1;
    CloudConnectStatus expected_get_resource_value_status_2;
};

const static std::vector<SetResourcesValueEntry> set_resources_values_entry_vector = {
    
    // Test valid scenario
    {
        VALID_APP_RESOURCE_DEFINITION_ONE_DYNAMIC_OBJECT_WITH_ONE_OBJECT_INSTANCE_AND_TWO_RESOURCE,
        // Set resource data
        mbl::ResourceData("/8888/11/111", "my_test_string"), // res 1
        mbl::ResourceData("/8888/11/112", 556677), // res 2
        CloudConnectStatus::STATUS_SUCCESS, // expected set resource_status
        ipc_conn_handle,
        CloudConnectStatus::STATUS_SUCCESS, // res 1
        CloudConnectStatus::STATUS_SUCCESS, // res 2
    },

    // Test setting value of string to an integer resource and vica versa
    {
        VALID_APP_RESOURCE_DEFINITION_ONE_DYNAMIC_OBJECT_WITH_ONE_OBJECT_INSTANCE_AND_TWO_RESOURCE,
        // Set resource data
        mbl::ResourceData("/8888/11/111", 123), // should be a string
        mbl::ResourceData("/8888/11/112", "this_is_not_an_integer"), // should be integer
        CloudConnectStatus::STATUS_SUCCESS, // expected set resource_status
        // Expected set resource data
        CloudConnectStatus::ERR_INVALID_RESOURCE_TYPE, // res 1
        CloudConnectStatus::ERR_INVALID_RESOURCE_TYPE, // res 2
    },

    // Test setting value resource that wasn't registered
    {
        VALID_APP_RESOURCE_DEFINITION_ONE_DYNAMIC_OBJECT_WITH_ONE_OBJECT_INSTANCE_AND_TWO_RESOURCE,
        // Set resource data
        mbl::ResourceData("/9999/11/111", "string_value"), //no such resource
        mbl::ResourceData("/8888/99/112", 555), //no such resource
        CloudConnectStatus::STATUS_SUCCESS, // expected set resource_status
        // Expected set resource data
        CloudConnectStatus::ERR_RESOURCE_NOT_FOUND, // res 1
        CloudConnectStatus::ERR_RESOURCE_NOT_FOUND, // res 2
    },

    // Test setting value resource with invalid path
    {
        VALID_APP_RESOURCE_DEFINITION_ONE_DYNAMIC_OBJECT_WITH_ONE_OBJECT_INSTANCE_AND_TWO_RESOURCE,
        // Set resource data
        mbl::ResourceData("8888/11/111", "string_value"), // path should start with "/"
        mbl::ResourceData("/8888/11", 555), // path not 3 level depth
        ipc_conn_handle,
        CloudConnectStatus::STATUS_SUCCESS, // expected set resource_status
        // Expected set resource data
        CloudConnectStatus::ERR_INVALID_RESOURCE_PATH, // res 1
        CloudConnectStatus::ERR_INVALID_RESOURCE_PATH, // res 2
    },

    // Test setting value resource with invalid access token
    {
        VALID_APP_RESOURCE_DEFINITION_ONE_DYNAMIC_OBJECT_WITH_ONE_OBJECT_INSTANCE_AND_TWO_RESOURCE,
        // Set resource data
        mbl::ResourceData("8888/11/111", "string_value"), // path should start with "/"
        mbl::ResourceData("/8888/11", 555), // path not 3 level depth
        CloudConnectStatus::ERR_INVALID_ACCESS_TOKEN, // expected set resource_status
        // Expected set resource data
        CloudConnectStatus::STATUS_SUCCESS, // ignored in this test as access token is invalid
        CloudConnectStatus::STATUS_SUCCESS, // ignored in this test as access token is invalid
    },    
};

/**
 * @brief Test resource broker set_resources_values API
 * 
 * The tested scenario is:
 * 1. Register 2 resources
 * 2. Set resource value of these 2 resources
 * 3. Compare with expectred results
 */
class ResourceBrokerSetResourceTest : public testing::TestWithParam<int>{};
INSTANTIATE_TEST_CASE_P(SetResourcesValueParameterizedTest,
                        ResourceBrokerSetResourceTest,
                        ::testing::Range(0, (int)set_resources_values_entry_vector.size(), 1));
TEST_P(ResourceBrokerSetResourceTest, set_resources_values)
{
    const SetResourcesValueEntry &test_data = set_resources_values_entry_vector[(size_t)GetParam()];

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

    std::vector<mbl::ResourceSetOperation> inout_set_operations;
    mbl::ResourceSetOperation res_set_op_1(test_data.set_resource_data_1);
    mbl::ResourceSetOperation res_set_op_2(test_data.set_resource_data_2);
    inout_set_operations.push_back(res_set_op_1);
    inout_set_operations.push_back(res_set_op_2);

    std::vector<mbl::ResourceSetOperation> expected_inout_set_operations = inout_set_operations;
    expected_inout_set_operations[0].output_status_ = test_data.expected_get_resource_value_status_1;
    expected_inout_set_operations[1].output_status_ = test_data.expected_get_resource_value_status_2;

    std::string test_access_token = out_access_token;
    if(test_data.expected_set_resource_status == 
        CloudConnectStatus::ERR_INVALID_ACCESS_TOKEN) 
    {
        test_access_token = "dummy_access_token";
    }
    // Set resources value
    resource_broker_tester.set_resources_values_test(
        test_access_token,
        inout_set_operations,
        expected_inout_set_operations,
        test_data.expected_set_resource_status
    );
}
