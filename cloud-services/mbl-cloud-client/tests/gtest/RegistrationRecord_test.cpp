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
#include "RegistrationRecordTester.h"
#include "RegistrationRecord.h"
#include "CloudConnectTrace.h"
#include "CloudConnectTypes.h"
#include "TestInfra.h"

#define TRACE_GROUP "ccrb-set_res_value-test"

struct ResourceIdentifiersEntry{
    const std::string resource_path;
    mbl::MblError expected_error_status;
    const std::string expected_object_name;
    uint16_t expected_object_instance_id;
    const std::string expected_resource_name;
    const std::string expected_resource_instance_name;
};

const static std::vector<ResourceIdentifiersEntry> resource_identifiers_entry_vector = {
    {
        "/8888/11/111",
        mbl::Error::None,
        "8888",
        11,
        "111",
        ""
    },
    {
        "/8888/11/112",
        mbl::Error::None,
        "8888",
        11,
        "112",
        ""
    },
    {
        "/8888", // Path should be 3 level depth
        mbl::MblError::CCRBInvalidResourcePath,
        "",
        0,
        "",
        ""
    },
    {
        "/8888/11", // Path should be 3 level depth
        mbl::MblError::CCRBInvalidResourcePath,
        "",
        0,
        "",
        ""
    },
    {
        "/8888/11/111/1/2/3", // Path should be 3 level depth
        mbl::MblError::CCRBInvalidResourcePath,
        "",
        0,
        "",
        ""
    },
    {
        "/8888/11abc/111", // Object instance should be a number
        mbl::MblError::CCRBInvalidResourcePath,
        "",
        0,
        "",
        ""        
    },
    {
        "/8888/abc11/111", // Object instance should be a number
        mbl::MblError::CCRBInvalidResourcePath,
        "",
        0,
        "",
        ""        
    },
    {
        "//////8888/11/111", // Invalid prefix
        mbl::MblError::CCRBInvalidResourcePath,
        "",
        0,
        "",
        ""        
    },
    {
        "/8888//11/111", // Two subsequent "/"
        mbl::MblError::CCRBInvalidResourcePath,
        "",
        0,
        "",
        ""        
    },
    {
        "/8888/11/111/", // Path should not end with "/"
        mbl::MblError::CCRBInvalidResourcePath,
        "",
        0,
        "",
        ""        
    },    
};

class RegistrationRecordTest : public testing::TestWithParam<int>{};
INSTANTIATE_TEST_CASE_P(RegistrationRecordParameterizedTest,
                        RegistrationRecordTest,
                        ::testing::Range(0, (int)resource_identifiers_entry_vector.size(), 1));

/**
 * @brief This parameterized test check get resource identifiers using valid / invalid paths
 * Resource identifiers are object name, object instance id, resource name and resource instance id.
 */
TEST_P(RegistrationRecordTest, get_resource_identifiers)
{
    GTEST_LOG_START_TEST;

    const ResourceIdentifiersEntry &test_data = 
        resource_identifiers_entry_vector[(size_t)GetParam()];

    RegistrationRecordTester registration_record_tester;
    registration_record_tester.get_resource_identifiers_test(
        test_data.resource_path,
        test_data.expected_error_status,
        test_data.expected_object_name,
        test_data.expected_object_instance_id,
        test_data.expected_resource_name,
        test_data.expected_resource_instance_name);
}

/**
 * @brief Test track_ipc_connection API return values:
 *              Error::CCRBConnectionNotFound - In case source IPC connection is closed but
 *                                              not found in connection vector.
 *              Error::CCRBNoValidConnection -  In case connection vector is empty after remove
 *                                              of source connection. This will signal CCRB to 
 *                                              erase this instance of RegistrationRecord.
 *              Error::None - in case of success
 */
TEST(RegistrationRecord, track_ipc_connection_test) 
{
    mbl::IpcConnection source_1("source_1"), source_2("source_2"), source_not_used("not_used");
    mbl::RegistrationRecord registration_record(source_1);
    mbl::MblError status = registration_record.track_ipc_connection(source_1,
        mbl::RegistrationRecord::TrackOperation::ADD);
    ASSERT_TRUE(mbl::MblError::None == status);

    // Add new ipc connection
    status = registration_record.track_ipc_connection(source_2,
        mbl::RegistrationRecord::TrackOperation::ADD);
    ASSERT_TRUE(mbl::MblError::None == status);

    // source_1 is closed
    status = registration_record.track_ipc_connection(source_1,
        mbl::RegistrationRecord::TrackOperation::REMOVE);
    ASSERT_TRUE(mbl::MblError::None == status);

    // source_not_used is closed
    status = registration_record.track_ipc_connection(source_not_used,
        mbl::RegistrationRecord::TrackOperation::REMOVE);
    ASSERT_TRUE(mbl::MblError::CCRBConnectionNotFound == status);

    // source_2 is closed
    status = registration_record.track_ipc_connection(source_2,
        mbl::RegistrationRecord::TrackOperation::REMOVE);
    ASSERT_TRUE(mbl::MblError::CCRBNoValidConnection == status);
}
