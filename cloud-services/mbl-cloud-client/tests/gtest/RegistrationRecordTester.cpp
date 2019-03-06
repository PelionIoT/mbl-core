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

#include "RegistrationRecordTester.h"
#include "RegistrationRecord.h"
#include "CloudConnectTrace.h"
#include <gtest/gtest.h>

#define TRACE_GROUP "ccrb-reg-record-tester"

void RegistrationRecordTester::get_resource_identifiers_test(
    const std::string& path,
    mbl::MblError expected_error_status,
    const std::string& expected_object_name,
    uint16_t expected_object_instance_id,
    const std::string& expected_resource_name,
    const std::string& expected_resource_instance_name)
{
    std::string object_name, resource_name, resource_instance_name;
    uint16_t object_instance_id = 0;
    const mbl::MblError status = mbl::RegistrationRecord::get_resource_identifiers(
        path,
        object_name,
        object_instance_id,
        resource_name, resource_instance_name);
    ASSERT_TRUE(expected_error_status == status);
    if(mbl::MblError::None == status) {
        ASSERT_TRUE(expected_object_name == object_name);
        ASSERT_TRUE(expected_object_instance_id == object_instance_id);
        ASSERT_TRUE(expected_resource_name == resource_name);
        ASSERT_TRUE(expected_resource_instance_name == resource_instance_name);

    }
}
