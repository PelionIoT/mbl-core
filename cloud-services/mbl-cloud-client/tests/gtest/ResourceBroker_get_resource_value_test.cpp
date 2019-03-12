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
#include "TestInfra.h"
#include "ResourceBrokerTester.h"
#include "ResourceDefinitionJson.h"

#define TRACE_GROUP "ccrb-get_res_value-test"

struct GetResourcesValueEntry{
     // Register resources
    const std::string application_resource_definition;
    // Set resource data
    mbl::ResourceData set_resource_data_1;
    mbl::ResourceData set_resource_data_2;
    // Get resources inout data
    mbl::ResourceGetOperation inout_get_resource_operation_1;
    mbl::ResourceGetOperation inout_get_resource_operation_2;
    CloudConnectStatus expected_get_resources_value_status;
    // Expected get resources data
    mbl::ResourceGetOperation expected_get_resource_operation_1;
    mbl::ResourceGetOperation expected_get_resource_operation_2;
    CloudConnectStatus expected_get_resource_value_status_1;
    CloudConnectStatus expected_get_resource_value_status_2;
};

const static std::vector<GetResourcesValueEntry> get_resources_values_entry_vector = {
    
    // Test valid scenario
    {
        VALID_APP_RESOURCE_DEFINITION_ONE_DYNAMIC_OBJECT_WITH_ONE_OBJECT_INSTANCE_AND_TWO_RESOURCE,
        // Set resource data
        mbl::ResourceData("/8888/11/111", "my_test_string"), // res 1
        mbl::ResourceData("/8888/11/112", 556677), // res 2
        // Get resource data
        mbl::ResourceGetOperation("/8888/11/111", ResourceDataType::STRING), // res 1
        mbl::ResourceGetOperation("/8888/11/112", ResourceDataType::INTEGER),// res 2 
        CloudConnectStatus::STATUS_SUCCESS, // expected get resources value status
        // Expected get resource data
        mbl::ResourceGetOperation("/8888/11/111", ResourceDataType::STRING), // res 1
        mbl::ResourceGetOperation("/8888/11/112", ResourceDataType::INTEGER),// res 2
        CloudConnectStatus::STATUS_SUCCESS, // expected get resource value status 1
        CloudConnectStatus::STATUS_SUCCESS, // expected get resource value status 2
    },

    // Test get value of string while resource type is integer and vica versa
    {
        VALID_APP_RESOURCE_DEFINITION_ONE_DYNAMIC_OBJECT_WITH_ONE_OBJECT_INSTANCE_AND_TWO_RESOURCE,
        // Set resource data
        mbl::ResourceData("/8888/11/111", "my_test_string"), // res 1
        mbl::ResourceData("/8888/11/112", 556677), // res 2
        // Get resource data
        mbl::ResourceGetOperation("/8888/11/111", ResourceDataType::INTEGER), // res 1 (should be string)
        mbl::ResourceGetOperation("/8888/11/112", ResourceDataType::STRING),// res 2 (should be integer) 
        CloudConnectStatus::STATUS_SUCCESS, // expected get resources value status
        // Expected get resource data
        mbl::ResourceGetOperation("/8888/11/111", ResourceDataType::STRING), // res 1
        mbl::ResourceGetOperation("/8888/11/112", ResourceDataType::INTEGER),// res 2
        CloudConnectStatus::ERR_INVALID_RESOURCE_TYPE, // expected get resource value status 1
        CloudConnectStatus::ERR_INVALID_RESOURCE_TYPE, // expected get resource value status 2
    },

    // Test get value of resource that wasn't registered
    {
        VALID_APP_RESOURCE_DEFINITION_ONE_DYNAMIC_OBJECT_WITH_ONE_OBJECT_INSTANCE_AND_TWO_RESOURCE,
        // Set resource data
        mbl::ResourceData("/8888/11/111", "my_test_string"), // res 1
        mbl::ResourceData("/8888/11/112", 556677), // res 2
        // Get resource data
        mbl::ResourceGetOperation("/8888/11/555", ResourceDataType::INTEGER), // res 1 (no such res)
        mbl::ResourceGetOperation("/8888/555/112", ResourceDataType::STRING), // res 2 (no such res) 
        CloudConnectStatus::STATUS_SUCCESS, // expected get resources value status
        // Expected get resource data
        mbl::ResourceGetOperation("/8888/11/555", ResourceDataType::STRING),  // res 1 (no such res)
        mbl::ResourceGetOperation("/8888/555/112", ResourceDataType::INTEGER),// res 2 (no such res)
        CloudConnectStatus::ERR_RESOURCE_NOT_FOUND, // expected get resource value status  1
        CloudConnectStatus::ERR_RESOURCE_NOT_FOUND, // expected get resource value status  2
    },

    // Test get value resource with invalid path
    {
        VALID_APP_RESOURCE_DEFINITION_ONE_DYNAMIC_OBJECT_WITH_ONE_OBJECT_INSTANCE_AND_TWO_RESOURCE,
        // Set resource data
        mbl::ResourceData("/8888/11/111", "my_test_string"), // res 1
        mbl::ResourceData("/8888/11/112", 556677), // res 2
        // Get resource data
        mbl::ResourceGetOperation("/8888/1aaaaa1/111", ResourceDataType::STRING), // res 1 (invalid path)
        mbl::ResourceGetOperation("/8888//111", ResourceDataType::INTEGER), // res 2 (invalid path)
        CloudConnectStatus::STATUS_SUCCESS, // expected get resources value status
        // Expected get resource data
        mbl::ResourceGetOperation("/8888/1aaaaa1/111", ResourceDataType::STRING), // res 1 (invalid path)
        mbl::ResourceGetOperation("/8888//111", ResourceDataType::INTEGER), // res 2 (invalid path)
        CloudConnectStatus::ERR_INVALID_RESOURCE_PATH, // expected get resource value status  1
        CloudConnectStatus::ERR_INVALID_RESOURCE_PATH, // expected get resource value status  2
    },

    // Test get value resource invlaid access token
    {
        VALID_APP_RESOURCE_DEFINITION_ONE_DYNAMIC_OBJECT_WITH_ONE_OBJECT_INSTANCE_AND_TWO_RESOURCE,
        // Set resource data
        mbl::ResourceData("/8888/11/111", "my_test_string"), // res 1
        mbl::ResourceData("/8888/11/112", 556677), // res 2
        // Get resource data
        mbl::ResourceGetOperation("/8888/1aaaaa1/111", ResourceDataType::STRING), // res 1 (invalid path)
        mbl::ResourceGetOperation("/8888//111", ResourceDataType::INTEGER), // res 2 (invalid path)
        CloudConnectStatus::ERR_INVALID_ACCESS_TOKEN, // expected get resources value status
        // Expected get resource data
        mbl::ResourceGetOperation("/8888/1aaaaa1/111", ResourceDataType::STRING), // res 1 (invalid path)
        mbl::ResourceGetOperation("/8888//111", ResourceDataType::INTEGER), // res 2 (invalid path)
        CloudConnectStatus::STATUS_SUCCESS, // ignored in this test as access token is invalid
        CloudConnectStatus::STATUS_SUCCESS, // ignored in this test as access token is invalid
    },
};

/**
 * @brief Test resource broker get_resources_values API
 * 
 * The tested scenario is:
 * 1. Register 2 resources
 * 2. Set resource value of these 2 resources
 * 3. Get resource values and compare with expectred results
 */
class ResourceBrokerGetResourceTest : public testing::TestWithParam<int>{};
INSTANTIATE_TEST_CASE_P(GetResourcesValueParameterizedTest,
                        ResourceBrokerGetResourceTest,
                        ::testing::Range(0, (int)get_resources_values_entry_vector.size(), 1));
TEST_P(ResourceBrokerGetResourceTest, get_resources_values)
{
    const GetResourcesValueEntry &test_data = get_resources_values_entry_vector[(size_t)GetParam()];

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
    expected_inout_set_operations[0].output_status_ = CloudConnectStatus::STATUS_SUCCESS;
    expected_inout_set_operations[1].output_status_ = CloudConnectStatus::STATUS_SUCCESS;

    // Set resources value
    resource_broker_tester.set_resources_values_test(
        out_access_token,
        inout_set_operations,
        expected_inout_set_operations,
        CloudConnectStatus::STATUS_SUCCESS // expected cloud connect status
    );

    // Test get resources values
    std::vector<mbl::ResourceGetOperation> inout_get_operations;
    inout_get_operations.push_back(test_data.inout_get_resource_operation_1);
    inout_get_operations.push_back(test_data.inout_get_resource_operation_2);

    // Init expected values
    std::vector<mbl::ResourceGetOperation> expected_inout_get_operations;

    // Resource 1
    mbl::ResourceGetOperation expected_res_get_op_1 = test_data.expected_get_resource_operation_1;
    expected_res_get_op_1.output_status_ = test_data.expected_get_resource_value_status_1;
    if(test_data.set_resource_data_1.get_data_type() == ResourceDataType::STRING) {
        expected_res_get_op_1.inout_data_.set_value(
            test_data.set_resource_data_1.get_value_string());
    } else if(test_data.set_resource_data_1.get_data_type() == ResourceDataType::INTEGER) {
        expected_res_get_op_1.inout_data_.set_value(
            test_data.set_resource_data_1.get_value_integer());
    }
    expected_inout_get_operations.push_back(expected_res_get_op_1);

    // Resource 2
    mbl::ResourceGetOperation expected_res_get_op_2 = test_data.expected_get_resource_operation_2;
    expected_res_get_op_2.output_status_ = test_data.expected_get_resource_value_status_2;
    if(test_data.set_resource_data_2.get_data_type() == ResourceDataType::STRING) {
        expected_res_get_op_2.inout_data_.set_value(
            test_data.set_resource_data_2.get_value_string());
    } else if(test_data.set_resource_data_2.get_data_type() == ResourceDataType::INTEGER) {
        expected_res_get_op_2.inout_data_.set_value(
            test_data.set_resource_data_2.get_value_integer());
    }
    expected_inout_get_operations.push_back(expected_res_get_op_2);

    std::string test_access_token = out_access_token;
    if(test_data.expected_get_resources_value_status == 
        CloudConnectStatus::ERR_INVALID_ACCESS_TOKEN) 
    {
        test_access_token = "dummy_access_token";
    }

    // Test get resources value
    resource_broker_tester.get_resources_values_test(
        test_access_token,
        inout_get_operations,
        expected_inout_get_operations,
        test_data.expected_get_resources_value_status
    );
}

