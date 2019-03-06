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


#ifndef RegistrationRecordTester_h_
#define RegistrationRecordTester_h_

#include "MblError.h"
#include <string>

/**
 * @brief This class tests RegistrationRecord functionality
 * 
 * This class is a friend of RegistrationRecord class
 * and is therefore able to evaluate private members and test operations
 * 
 */
class RegistrationRecordTester {

public:

    /**
     * @brief Get resource identifiers test
     * 
     * @param path - resource path 
     * @param expected_error_status - Expected error status
     * @param expected_object_name - Expected object name
     * @param expected_object_instance_id - Expected object instance id
     * @param expected_resource_name - Expected resource name
     * @param expected_resource_instance_name - Expected resource instance name
     */
    void get_resource_identifiers_test(
        const std::string& path,
        mbl::MblError expected_error_status,
        const std::string& expected_object_name,
        uint16_t expected_object_instance_id,
        const std::string& expected_resource_name,
        const std::string& expected_resource_instance_name);
};

#endif // RegistrationRecordTester_h_
