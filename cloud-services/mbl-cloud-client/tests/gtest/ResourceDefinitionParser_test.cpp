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
#include "ResourceDefinitionJson.h"
#include "MblCloudClient.h"
#include "MblError.h"
#include "CloudConnectTrace.h"
#include "cloud-connect-resource-broker/ResourceDefinitionParser.h"
#include <cinttypes>

#define TRACE_GROUP "ccrb-resdefparser-test"

/**
 * @brief Check if M2MResource is equal to SPRBM2MResource
 * 
 * @param m2m_resource - M2MResource created when application resource definition JSON was parsed
 * @param sp_rbm2m_resource - SPRBM2MResource created when application resource definition JSON was parsed
 *                         OR created in google tests below to act as expected values to compare to
 * @param ignore_m2m_objects_compare - SPRBM2MResource contains pointers to M2MResource. These pointers are initialied
 *                                     when tje JSON was parsed. In case SPRBM2MResource was created using a test - these
 *                                     pointers will NOT be initialied and therefore need to be ignored.
 * @return true  - if M2MResource and SPRBM2MResource contain are equal (in term of values)
 * @return false - if M2MResource and SPRBM2MResource contain are NOT equal (in term of values)
 */
static void check_equal_resources(M2MResource* m2m_resource, M2MResource* m2m_resource_test)
{
    TR_DEBUG("Enter");

    // Compare resource names
    ASSERT_STREQ(m2m_resource->name(), m2m_resource_test->name());
    TR_DEBUG("Compare resource name succeeded (%s)", m2m_resource->name());

    // Compare mode
    ASSERT_TRUE(m2m_resource->mode() == m2m_resource_test->mode());
    TR_DEBUG("Compare mode succeeded (%d)", static_cast<int>(m2m_resource_test->mode()));

    // Compare type
    ASSERT_EQ(m2m_resource->resource_instance_type(), m2m_resource_test->resource_instance_type());
    TR_DEBUG("Compare type succeeded (%d)", static_cast<int>(m2m_resource->resource_instance_type()));
    
    if(m2m_resource->mode() == M2MBase::Dynamic) {
        // Compare observable (only dynamic resources have this entry)
        ASSERT_TRUE(m2m_resource->is_observable() == m2m_resource_test->is_observable());
        TR_DEBUG("Compare observable succeeded (%d)", m2m_resource_test->is_observable());
    } else if (m2m_resource->mode() == M2MBase::Static){
        // Compare value (JSON conatins value only for static resources)
        if(m2m_resource->resource_instance_type() == M2MResourceInstance::INTEGER) 
        {
            EXPECT_TRUE(m2m_resource->get_value_int() == m2m_resource_test->get_value_int());
        } else if(m2m_resource->resource_instance_type() == M2MResourceInstance::STRING) 
        {
            ASSERT_STREQ(m2m_resource->get_value_string().c_str(), m2m_resource_test->get_value_string().c_str());
        }
        TR_DEBUG("Compare value succeeded (%s)", m2m_resource->get_value_string().c_str());
    }

    // Compare supports multiple instances
    ASSERT_EQ(m2m_resource->supports_multiple_instances(), m2m_resource_test->supports_multiple_instances());
    TR_DEBUG("Compare supports multiple instances succeeded (%d)", m2m_resource->supports_multiple_instances());

    //Compare resource_type
    if(m2m_resource->resource_type() != nullptr) {
        ASSERT_STREQ(m2m_resource->resource_type(), m2m_resource_test->resource_type());
        TR_DEBUG("Compare resource type succeeded (%s)", m2m_resource->resource_type());
    }
    
    // Copare operation
    ASSERT_EQ(m2m_resource->operation(), m2m_resource_test->operation());
    TR_DEBUG("Compare operation succeeded (%d)", static_cast<int>(m2m_resource->operation()));
}

/**
 * @brief Check if M2MObjectInstance is equal to SPRBM2MObjectInstance
 * 
 * @param m2m_object_instance - M2MObjectInstance created when application resource definition JSON was parsed
 * @param m2m_object_instance_test - M2MObjectInstance test
 * @return true  - if m2m_object_instance and m2m_object_instance_test are equal
 * @return false - if m2m_object_instance and m2m_object_instance_test are NOT equal
 */
static void check_equal_object_instances(M2MObjectInstance* m2m_object_instance, M2MObjectInstance* m2m_object_instance_test)
{
    TR_DEBUG("Enter");

    // Compare number of resources under each object instance
    ASSERT_TRUE(m2m_object_instance->resources().size() == 
        m2m_object_instance_test->resources().size());

    // Iterate all resources
    M2MResource *m2m_resource = nullptr;
    M2MResource *m2m_resource_test = nullptr;
    M2MResourceList m2m_resource_list = m2m_object_instance->resources();

    for(auto& itr : m2m_resource_list)
    {
        m2m_resource = itr;
        m2m_resource_test = m2m_object_instance_test->resource(m2m_resource->name());
        TR_DEBUG("m2m_resource name: %s", m2m_resource->name());
        check_equal_resources(m2m_resource, m2m_resource_test);
    }
}

/**
 * @brief Check if M2MObject is equal to SPRBM2MObject
 * 
 * @param m2m_object - M2MObject created when application resource definition JSON was parsed
 * @param m2m_object_test - M2MObject test
 * @return true  - if m2m_object and m2m_object_test are equal
 * @return false - if m2m_object and m2m_object_test are NOT equal
 */
static void CheckEqualObject(M2MObject* m2m_object, M2MObject* m2m_object_test)
{
    TR_DEBUG("Enter");
    
    // Compare number of object instances under each object
    ASSERT_TRUE(m2m_object->instance_count() == m2m_object_test->instance_count());

    // Iterate all object instances
    M2MObjectInstance* m2m_object_instance = nullptr;
    M2MObjectInstance* m2m_object_instance_test = nullptr;
    M2MObjectInstanceList m2m_object_instance_list = m2m_object->instances();
    for(auto& itr : m2m_object_instance_list)
    {
        m2m_object_instance = itr;
        m2m_object_instance_test = 
            m2m_object_test->object_instance(m2m_object_instance->instance_id());
        TR_DEBUG("m2m_object_instance id: %" PRId16, m2m_object_instance->instance_id());
        check_equal_object_instances(m2m_object_instance, m2m_object_instance_test);
    }
}

/**
 * @brief Check if M2MObject is equal to SPRBM2MObject
 * 
 * @param m2m_object_list - M2MObjectList created when application resource definition JSON was parsed
 * @param m2m_object_list_test - M2MObjectList test list
 * @return true  - if m2m_object_list and m2m_object_list_test are equal
 * @return false - if m2m_object_list and m2m_object_list_test are NOT equal
 */
static void check_equal_object_lists(const M2MObjectList &m2m_object_list, const M2MObjectList &m2m_object_list_test)
{
    TR_DEBUG("Enter");

    // Compare number of object instances under each object
    ASSERT_TRUE(m2m_object_list.size() == m2m_object_list_test.size());

    // Iterate all Objects
    bool found_obj_test = false;
    M2MObject *m2m_object = nullptr;
    M2MObject *m2m_object_test = nullptr;    
    for(auto& itr : m2m_object_list)
    {
        m2m_object = itr;
        std::string object_name = m2m_object->name();
        TR_DEBUG("object_name: %s", object_name.c_str());

        found_obj_test = false;
        for(auto& itr_test : m2m_object_list_test)
        {
            m2m_object_test = itr_test;
            std::string object_test_name = m2m_object_test->name();
            // Object name are unique as we use strict JSON parsing
            if(object_name == object_test_name) {
                found_obj_test = true;
                break;
            }
        }
        ASSERT_TRUE(found_obj_test);
        CheckEqualObject(m2m_object, m2m_object_test);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//Positive Tests:
////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(JsonTest_Positive, Objects_with_several_object_instances_and_resources) {

    TR_DEBUG("Enter");

    M2MObjectList m2m_object_list;
    const std::string app_resource_definition =
        VALID_APP_RESOURCE_DEFINITION_OBJECT_WITH_SEVERAL_OBJECT_INSTANCES_AND_RESOURCES;

    ASSERT_TRUE(
        mbl::ResourceDefinitionParser::build_object_list(app_resource_definition, m2m_object_list) 
        == mbl::Error::None);

    // m2m_object_list_test (contains object_1 and object_2)
    M2MObjectList m2m_object_list_test;

    // object_1
    auto object_1 = M2MInterfaceFactory::create_object("1");
    m2m_object_list_test.push_back(object_1);

    // object_1 (contains object_instance_11)
    auto object_instance_11 = object_1->create_object_instance(11);
    
    // object_instance_11 (contains resource_111 and resource_112)
    
    //resource_111
    std::string resource_value = "string_val";
    uint8_t value_length = static_cast<uint8_t>(resource_value.length());
    auto value = new uint8_t[resource_value.length()];
    memmove(value, resource_value.data(), resource_value.length());
    auto m2m_resource_111 = object_instance_11->create_static_resource(
        "111", // Resource name
        "reset_button", // Resource type
        M2MResourceInstance::STRING, // Type
        value,
        value_length,
        false // Supports multiple instances
    );
    delete[] value;
    m2m_resource_111->set_operation(M2MBase::GET_ALLOWED);

    //resource_112
    auto m2m_resource_112 = object_instance_11->create_dynamic_resource(
        "112", // Resource name
        "", // Resource type
        M2MResourceInstance::STRING, // Type
        true, // Observable
        true  // Supports multiple instances
    );
    m2m_resource_112->set_operation(M2MBase::GET_PUT_DELETE_ALLOWED);

    // object_2
    auto object_2 = M2MInterfaceFactory::create_object("2");
    m2m_object_list_test.push_back(object_2);

    // object_2 (contains object_instance_21 and object_instance_22)

    //object_instance_21
    auto object_instance_21 = object_2->create_object_instance(21);

    // Object instance 21 (contains resource_211)
	
    resource_value = "999";
    value_length = static_cast<uint8_t>(resource_value.length());
    value = new uint8_t[resource_value.length()];
    memmove(value, resource_value.data(), resource_value.length());
    auto m2m_resource_211 = object_instance_21->create_static_resource(
        "211", // Resource name
        "", // Resource type
        M2MResourceInstance::INTEGER, // Type
        value,
        value_length,
        true // Supports multiple instances
    );
    delete[] value;
    m2m_resource_211->set_operation(M2MBase::GET_ALLOWED);

    // object_instance_22
    auto object_instance_22 = object_2->create_object_instance(22);

    // Object instance 22 (contains resource_221)
    auto m2m_resource_221 = object_instance_22->create_dynamic_resource(
        "221", // Resource name
        "", // Resource type
        M2MResourceInstance::INTEGER, // Type
        true, // Observable
        true // Supports multiple instances
    );
    m2m_resource_221->set_operation(M2MBase::GET_PUT_POST_ALLOWED);

    // Compare m2m_object_list with m2m_object_list_test
    check_equal_object_lists(m2m_object_list, m2m_object_list_test);
}

TEST(JsonTest_Positive, Two_objects_with_one_object_instances_and_one_resource) {
    
    TR_DEBUG("Enter");

    M2MObjectList m2m_object_list;
    std::string app_resource_definition =
        VALID_APP_RESOURCE_DEFINITION_TWO_OBJECTS_WITH_ONE_OBJECT_INSTANCE_AND_ONE_RESOURCE;

    ASSERT_TRUE(
        mbl::ResourceDefinitionParser::build_object_list(app_resource_definition, m2m_object_list) 
        == mbl::Error::None);

    // m2m_object_list_test (contains object_1 and object_2)
    M2MObjectList m2m_object_list_test;

    // Object 1
    auto object_1 = M2MInterfaceFactory::create_object("1");
    m2m_object_list_test.push_back(object_1);

    // Object 1 (contains object_instance_11)
    auto object_instance_11 = object_1->create_object_instance(11);

    // Object instance 11 (contains resource_111)

    std::string resource_value = "string_val";
    uint8_t value_length = static_cast<uint8_t>(resource_value.length());
    auto value = new uint8_t[resource_value.length()];
    memmove(value, resource_value.data(), resource_value.length());
    auto m2m_resource_111 = object_instance_11->create_static_resource(
        "111", // Resource name
        "reset_button", // Resource type
        M2MResourceInstance::STRING, // Type
        value,
        value_length,
        false // Supports multiple instances
    );
    delete[] value;
    m2m_resource_111->set_operation(M2MBase::GET_ALLOWED);

    // Object 2
    auto object_2 = M2MInterfaceFactory::create_object("2");
    m2m_object_list_test.push_back(object_2);

    // Object 2 (contains object_instance_21)
    auto object_instance_21 = object_2->create_object_instance(21);

    // Object instance 21 (contains resource_211)
    resource_value = "123456";
    value_length = static_cast<uint8_t>(resource_value.length());
    value = new uint8_t[resource_value.length()];
    memmove(value, resource_value.data(), resource_value.length());
    auto m2m_resource_211 = object_instance_21->create_static_resource(
        "211", // Resource name
        "", // Resource type
        M2MResourceInstance::INTEGER, // Type
        value,
        value_length,
        true // Supports multiple instances
    );
    delete[] value;
    m2m_resource_211->set_operation(M2MBase::GET_ALLOWED);

    // Compare m2m_object_list with m2m_object_list_test
    check_equal_object_lists(m2m_object_list, m2m_object_list_test);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Negative Tests:
////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(JsonTest_Negative, Invalid_app_resource_definition_not_3_Level) {
    // Valid JSON includes 3 levels per node (Object/Object Instance and resource).
    // This function verifies that parsing fails in case it doesn't
    TR_DEBUG("Enter");

    const std::string app_resource_definition = INVALID_APP_RESOURCE_DEFINITION_NOT_3_LEVEL_1;
    M2MObjectList m2m_object_list;

    ASSERT_TRUE(
        mbl::ResourceDefinitionParser::build_object_list(app_resource_definition, m2m_object_list) 
        == mbl::Error::CCRBInvalidJson);

    const std::string app_resource_definition_2 = INVALID_APP_RESOURCE_DEFINITION_NOT_3_LEVEL_2;

    ASSERT_TRUE(
        mbl::ResourceDefinitionParser::build_object_list(app_resource_definition_2 ,m2m_object_list) 
        == mbl::Error::CCRBInvalidJson);
}

TEST(JsonTest_Negative, Invalid_app_resource_definition_missing_Observable) {
    // In a valid JSON - "observable" entry is mandatory in Dynamic resource
    // This function verifies that parsing fails in case it doesn't includes it.
    TR_DEBUG("Enter");

    const std::string app_resource_definition = INVALID_APP_RESOURCE_DEFINITION_MISSING_OBSERVABLE_ENTRY;
    M2MObjectList m2m_object_list;

    ASSERT_TRUE(
        mbl::ResourceDefinitionParser::build_object_list(app_resource_definition, m2m_object_list) 
        == mbl::Error::CCRBInvalidJson);
}

TEST(JsonTest_Negative, Illegal_resource_mode) {
    // Valid JSON includes either "dynamic" or "static" modes.
    // This function verifies that parsing fails in case of invalid mode.
    TR_DEBUG("Enter");

    const std::string app_resource_definition = INVALID_APP_RESOURCE_DEFINITION_ILLEGAL_RESOURCE_MODE;
    M2MObjectList m2m_object_list;

    ASSERT_TRUE(
        mbl::ResourceDefinitionParser::build_object_list(app_resource_definition, m2m_object_list) 
        == mbl::Error::CCRBInvalidJson);
}

TEST(JsonTest_Negative, Illegal_resource_operation) { 
    // Valid JSON includes either "put" / "get" / "post" and "delete" as allowed operation
    // This function verifies that parsing fails in case of invalid operation.
    TR_DEBUG("Enter");

    const std::string app_resource_definition = INVALID_APP_RESOURCE_DEFINITION_ILLEGAL_RESOURCE_OPERATION;
    M2MObjectList m2m_object_list;

    ASSERT_TRUE(
        mbl::ResourceDefinitionParser::build_object_list(app_resource_definition, m2m_object_list) 
        == mbl::Error::CCRBInvalidJson);
}

TEST(JsonTest_Negative, Unsupported_resource_type) {
    // Valid JSON includes either "string" or "integer" resource types.
    // This function verifies that parsing fails in case of invalid resource type.
    TR_DEBUG("Enter");

    const std::string app_resource_definition = INVALID_APP_RESOURCE_DEFINITION_UNSUPPORTED_RESOURCE_TYPE;
    M2MObjectList m2m_object_list;

    ASSERT_TRUE(
        mbl::ResourceDefinitionParser::build_object_list(app_resource_definition, m2m_object_list) 
        == mbl::Error::CCRBInvalidJson);
}

TEST(JsonTest_Negative, Two_same_resource_names) {
    // Check that parsing of invalid JSON string with two same resource names fails
    TR_DEBUG("Enter");

    const std::string app_resource_definition = INVALID_APP_RESOURCE_DEFINITION_TWO_SAME_RESOURCE_NAMES;
    M2MObjectList m2m_object_list;

    ASSERT_TRUE(
        mbl::ResourceDefinitionParser::build_object_list(app_resource_definition, m2m_object_list) 
        == mbl::Error::CCRBInvalidJson);
}

TEST(JsonTest_Negative, Two_same_object_instances) {
    // Check that parsing of invalid JSON string with two same object instance ids fails
    TR_DEBUG("Enter");

    const std::string app_resource_definition = INVALID_APP_RESOURCE_DEFINITION_TWO_SAME_OBJECT_INSTANCES;
    M2MObjectList m2m_object_list;

    ASSERT_TRUE(
        mbl::ResourceDefinitionParser::build_object_list(app_resource_definition, m2m_object_list) 
        == mbl::Error::CCRBInvalidJson);
}

TEST(JsonTest_Negative, Two_same_objects) {
    // Check that parsing of invalid JSON string with two same object names fails
    TR_DEBUG("Enter");

    const std::string app_resource_definition = INVALID_APP_RESOURCE_DEFINITION_TWO_SAME_OBJECT;
    M2MObjectList m2m_object_list;

    ASSERT_TRUE(
        mbl::ResourceDefinitionParser::build_object_list(app_resource_definition, m2m_object_list) 
        == mbl::Error::CCRBInvalidJson);
}
