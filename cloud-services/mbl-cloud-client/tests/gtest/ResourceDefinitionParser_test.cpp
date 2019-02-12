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
#include "mbed-trace/mbed_trace.h"
#include "cloud-connect-resource-broker/ResourceDefinitionParser.h"

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
static void check_equal_resources(M2MResource *m2m_resource, const mbl::SPRBM2MResource sp_rbm2m_resource, bool ignore_m2m_objects_compare)
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    // Compare resource names
    ASSERT_STREQ(sp_rbm2m_resource->get_resource_name().c_str(), m2m_resource->name());
    tr_debug("Compare resource name succeeded (%s)", sp_rbm2m_resource->get_resource_name().c_str());

    // Make sure sp_rbm2m_resource points to m2m_resource:
    if(!ignore_m2m_objects_compare) {
        ASSERT_TRUE(sp_rbm2m_resource->get_m2m_resource() == m2m_resource);
        tr_debug("Compare to M2MResource succeeded (%p)", m2m_resource);    
    }
    // Compare mode
    ASSERT_TRUE(m2m_resource->mode() == sp_rbm2m_resource->get_mode());
    tr_debug("Compare mode succeeded (%d)", static_cast<int>(sp_rbm2m_resource->get_mode()));
    if(sp_rbm2m_resource->get_mode() == M2MBase::Dynamic) {
        // Compare observable (only dynamic resources have this entry)
        ASSERT_TRUE(m2m_resource->is_observable() == sp_rbm2m_resource->get_observable());
        tr_debug("Compare observable succeeded (%d)", sp_rbm2m_resource->get_observable());
    } else if (sp_rbm2m_resource->get_mode() == M2MBase::Static){
        // Compare value (JSON conatins value only for static resources)
        if(sp_rbm2m_resource->get_type() == M2MResourceInstance::INTEGER) 
        {
            int m2m_integer_value = (int)m2m_resource->get_value_int();
            int rbm2m2_integer_value = -1;
            ASSERT_TRUE(mbl::Error::None == sp_rbm2m_resource->get_value_as_integer(&rbm2m2_integer_value));
            EXPECT_TRUE(rbm2m2_integer_value == m2m_integer_value);
        } else if(sp_rbm2m_resource->get_type() == M2MResourceInstance::STRING) 
        {
            ASSERT_STREQ(sp_rbm2m_resource->get_value_as_string().c_str(), m2m_resource->get_value_string().c_str());
        }
        tr_debug("Compare value succeeded (%s)", sp_rbm2m_resource->get_value_as_string().c_str());
    }
    // Compare supports multiple instances
    ASSERT_EQ(m2m_resource->supports_multiple_instances(), sp_rbm2m_resource->get_supports_multiple_instances());
    tr_debug("Compare supports multiple instances succeeded (%d)", sp_rbm2m_resource->get_supports_multiple_instances());
    // Compare type
    ASSERT_EQ(m2m_resource->resource_instance_type(), sp_rbm2m_resource->get_type());
    tr_debug("Compare type succeeded (%d)", static_cast<int>(sp_rbm2m_resource->get_type()));
    //Compare resource_type
    if(m2m_resource->resource_type() != nullptr) {
        ASSERT_STREQ(m2m_resource->resource_type(), sp_rbm2m_resource->get_resource_type().c_str());
    } else {
        EXPECT_TRUE(sp_rbm2m_resource->get_resource_type().empty());
    }
    tr_debug("Compare resource type succeeded (%s)", sp_rbm2m_resource->get_resource_type().c_str());
    // Copare operation
    ASSERT_EQ(m2m_resource->operation(), sp_rbm2m_resource->get_operations());
    tr_debug("Compare operation succeeded (%d)", static_cast<int>(sp_rbm2m_resource->get_operations()));
}

/**
 * @brief Check if M2MObjectInstance is equal to SPRBM2MObjectInstance
 * 
 * @param m2m_object_instance - M2MObjectInstance created when application resource definition JSON was parsed
 * @param sp_rbm2m_object_instance - SPRBM2MObjectInstance created when application resource definition JSON was parsed
 *                                OR created in google tests below to act as expected values to compare to
 * @param ignore_m2m_objects_compare - SPRBM2MObjectInstance contains pointers to M2MObjectInstance. These pointers are initialied
 *                                     when tje JSON was parsed. In case SPRBM2MObjectInstance was created using a test - these
 *                                     pointers will NOT be initialied and therefore need to be ignored.
 * @return true  - if M2MObjectInstance and SPRBM2MObjectInstance contain are equal (in term of values)
 * @return false - if M2MObjectInstance and SPRBM2MObjectInstance contain are NOT equal (in term of values)
 */
static void check_equal_object_instances(M2MObjectInstance* m2m_object_instance, const mbl::SPRBM2MObjectInstance sp_rbm2m_object_instance, bool ignore_m2m_objects_compare)
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    // Compare number of resources under each object instance
    ASSERT_TRUE(m2m_object_instance->resources().size() == (int)sp_rbm2m_object_instance->get_resource_map().size());

    // Make sure sp_rbm2m_object_instance points to m2m_object_instance:
    if(!ignore_m2m_objects_compare) {
        ASSERT_TRUE(sp_rbm2m_object_instance->get_m2m_object_instance() == m2m_object_instance);
        tr_debug("Compare to M2MObjectInstance succeeded (%p)", m2m_object_instance);
    }

    // Iterate all resources
    M2MResource *m2m_resource = nullptr;
    mbl::SPRBM2MResource sp_rbm2m_resource = nullptr;
    for(auto& itr : sp_rbm2m_object_instance->get_resource_map())
    {
        std::string rbm2m_resource_name = itr.first;
        sp_rbm2m_resource = itr.second;
        tr_debug("rbm2m_resource_name: %s", rbm2m_resource_name.c_str());
        m2m_resource = m2m_object_instance->resource(rbm2m_resource_name.c_str());
        ASSERT_TRUE(m2m_resource != nullptr);
        check_equal_resources(m2m_resource, sp_rbm2m_resource, ignore_m2m_objects_compare);
    }
    tr_debug("%s - end.", __PRETTY_FUNCTION__);
}

/**
 * @brief Check if M2MObject is equal to SPRBM2MObject
 * 
 * @param m2m_object - M2MObject created when application resource definition JSON was parsed
 * @param sp_rbm2m_object - SPRBM2MObject created when application resource definition JSON was parsed
 *                       OR created in google tests below to act as expected values to compare to
 * @param ignore_m2m_objects_compare - SPRBM2MObject contains pointers to M2MObject. These pointers are initialied
 *                                     when tje JSON was parsed. In case SPRBM2MObject was created using a test - these
 *                                     pointers will NOT be initialied and therefore need to be ignored.
 * @return true  - if M2MObject and SPRBM2MObject contain are equal (in term of values)
 * @return false - if M2MObject and SPRBM2MObject contain are NOT equal (in term of values)
 */
static void CheckEqualObject(M2MObject* m2m_object, mbl::SPRBM2MObject sp_rbm2m_object, bool ignore_m2m_objects_compare)
{
    // Compare number of object instances under each object
    ASSERT_TRUE(m2m_object->instance_count() == (sp_rbm2m_object)->get_object_instance_map().size());

    // Make sure sp_rbm2m_object points to m2m_object:
    if(!ignore_m2m_objects_compare) {
        ASSERT_TRUE((sp_rbm2m_object)->get_m2m_object() == m2m_object);
        tr_debug("Compare to M2MObjects succeeded (%p)", m2m_object);
    }

    // Iterate all object instances
    M2MObjectInstance* m2m_object_instance = nullptr;
    mbl::SPRBM2MObjectInstance sp_rbm2m_object_instance = nullptr;
    for(auto& itr : (sp_rbm2m_object)->get_object_instance_map())
    {
        uint16_t rbm2m_object_instance_id = itr.first;
        sp_rbm2m_object_instance = itr.second;
        tr_debug("rbm2m_object_instance_id: %d", rbm2m_object_instance_id);
        m2m_object_instance = m2m_object->object_instance(rbm2m_object_instance_id);
        check_equal_object_instances(m2m_object_instance, sp_rbm2m_object_instance, ignore_m2m_objects_compare);
    }
}

/**
 * @brief Check if M2MObject is equal to SPRBM2MObject
 * 
 * @param m2m_object_list - M2MObjectList created when application resource definition JSON was parsed
 * @param rbm2m_object_list - RBM2MObjectList created when application resource definition JSON was parsed
 *                            OR created in google tests below to act as expected values to compare to
 * @param ignore_m2m_objects_compare - In case RBM2MObjectList was created using a test - set to true, else - set to false
 * @return true  - if M2MObjectList and RBM2MObjectList contain are equal (in term of values)
 * @return false - if M2MObjectList and RBM2MObjectList contain are NOT equal (in term of values)
 */
static void check_equal_object_lists(const M2MObjectList &m2m_object_list, mbl::RBM2MObjectList &rbm2m_object_list, bool ignore_m2m_objects_compare)
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    // Compare number of object instances under each object
    ASSERT_TRUE(m2m_object_list.size() == (int)rbm2m_object_list.get_object_map().size());

    // Iterate all Objects
    M2MObject *m2m_object = nullptr;
    mbl::SPRBM2MObject sp_rbm2m_object = nullptr;
    for(auto& itr : m2m_object_list)
    {
        m2m_object = itr;
        std::string object_name = m2m_object->name();
        tr_debug("object_name: %s", object_name.c_str());
        sp_rbm2m_object = rbm2m_object_list.get_object(object_name);
        ASSERT_TRUE(sp_rbm2m_object != nullptr);
        CheckEqualObject(m2m_object, sp_rbm2m_object, ignore_m2m_objects_compare);
    }
}

////////////////////////////////////////////////////////////////////////////////
//Positive Tests:
TEST(JsonTest_Positive, Objects_with_several_object_instances_and_resources) {

    tr_debug("%s", __PRETTY_FUNCTION__);

    M2MObjectList m2m_object_list;
    mbl::RBM2MObjectList rbm2m_object_list;
    const std::string json_string = VALID_JASON_OBJECT_WITH_SEVERAL_OBJECT_INSTANCES_AND_RESOURCES;
    mbl::ResourceDefinitionParser resource_parser;
    ASSERT_TRUE(resource_parser.build_object_list(json_string, m2m_object_list, rbm2m_object_list) == mbl::Error::None);

    // RBM2MObjectList (contains object_1 and object_2)
    mbl::RBM2MObjectList rbm2m_object_list_test;
    // Object 1
    mbl::SPRBM2MObject object_1 = rbm2m_object_list_test.create_object("1");
    // Object 1 (contains object_instance_11)
    mbl::SPRBM2MObjectInstance object_instance_11 = object_1->create_object_instance(11);
    // Object instance 11 (contains resource_111 and resource_112)
    object_instance_11->create_resource(
        "111",                          // Resource name
        M2MBase::Static,                // M2MBase::Mode
        false,                          // Supports multiple instances
        M2MBase::GET_ALLOWED,           // Operation
        false,                          // Observable
        "reset_button",                 // Resource type
        M2MResourceInstance::STRING,    // Type
        "string_val"                  // Value
    );
    object_instance_11->create_resource(
        "112",                          // Resource name
        M2MBase::Dynamic,               // M2MBase::Mode
        true,                           // Supports multiple instances
        M2MBase::GET_PUT_DELETE_ALLOWED,// Operation
        true,                           // Observable
        "",                             // Resource type
        M2MResourceInstance::STRING,    // Type
        "N/A"                           // Value will be ignored as this is dynamic resource
    );

    // Object 2
    mbl::SPRBM2MObject object_2 = rbm2m_object_list_test.create_object("2");
    // Object 2 (contains object_instance_21 and object_instance_22)
    mbl::SPRBM2MObjectInstance object_instance_21 = object_2->create_object_instance(21);
    // Object instance 21 (contains resource_211)
    object_instance_21->create_resource(
        "211",                          // Resource name
        M2MBase::Static,                // M2MBase::Mode
        true,                           // Supports multiple instances
        M2MBase::GET_ALLOWED,           // Operation
        true,                           // Observable
        "",                             // Resource type
        M2MResourceInstance::INTEGER,   // Type
        "999"                           // Value
    );
    mbl::SPRBM2MObjectInstance object_instance_22 = object_2->create_object_instance(22);
    // Object instance 22 (contains resource_221)
    object_instance_22->create_resource(
        "221",                          // Resource name
        M2MBase::Dynamic,               // M2MBase::Mode
        true,                           // Supports multiple instances
        M2MBase::GET_PUT_POST_ALLOWED,  // Operation
        true,                           // Observable
        "",                             // Resource type
        M2MResourceInstance::INTEGER,   // Type
        "N/A"                           // Value will be ignored as this is dynamic resource
    );

    // Compare m2m2 object list with rbm2m test object list
    check_equal_object_lists(m2m_object_list, rbm2m_object_list_test, true);
    // We now know m2m_object_list is as expected
    // Compare rbm2m2 object list with rbm2m object list
    check_equal_object_lists(m2m_object_list, rbm2m_object_list, false);
}

TEST(JsonTest_Positive, Two_objects_with_one_object_instances_and_one_resource) {
    
    tr_debug("%s", __PRETTY_FUNCTION__);
    M2MObjectList m2m_object_list;
    mbl::RBM2MObjectList rbm2m_object_list;
    const std::string json_string = VALID_JASON_TWO_OBJECTS_WITH_ONE_OBJECT_INSTANCE_AND_ONE_RESOURCE;
    mbl::ResourceDefinitionParser resource_parser;
    ASSERT_TRUE(resource_parser.build_object_list(json_string, m2m_object_list, rbm2m_object_list) == mbl::Error::None);

    // RBM2MObjectList (contains object_1 and object_2)
    mbl::RBM2MObjectList rbm2m_object_list_test;
    // Object 1
    mbl::SPRBM2MObject object_1 = rbm2m_object_list_test.create_object("1");
    // Object 1 (contains object_instance_11)
    mbl::SPRBM2MObjectInstance object_instance_11 = object_1->create_object_instance(11);
    // Object instance 11 (contains resource_111)
    object_instance_11->create_resource(
        "111",                          // Resource name
        M2MBase::Static,                // M2MBase::Mode
        false,                          // Supports multiple instances
        M2MBase::GET_ALLOWED,           // Operation
        false,                          // Observable (Not relevant for static)
        "reset_button",                 // Resource type
        M2MResourceInstance::STRING,    // Type
        "string_val"                    // Value
    );

    // Object 2
    mbl::SPRBM2MObject object_2 = rbm2m_object_list_test.create_object("2");
    // Object 2 (contains object_instance_21)
    mbl::SPRBM2MObjectInstance object_instance_21 = object_2->create_object_instance(21);
    // Object instance 21 (contains resource_211)
    object_instance_21->create_resource(
        "211",                          // Resource name
        M2MBase::Static,                // M2MBase::Mode
        true,                           // Supports multiple instances
        M2MBase::GET_ALLOWED,           // Operation
        false,                          // Observable (Not relevant for static)
        "",                             // Resource type
        M2MResourceInstance::INTEGER,   // Type
        "123456"                        // Value
    );

    // Compare m2m2 object list with rbm2m test object list
    check_equal_object_lists(m2m_object_list, rbm2m_object_list_test, true);
    // We now know m2m_object_list is as expected
    // Compare rbm2m2 object list with rbm2m object list
    check_equal_object_lists(m2m_object_list, rbm2m_object_list, false);
}

////////////////////////////////////////////////////////////////////////////////
// Negative Tests:
TEST(JsonTest_Negative, Invalid_Json_Not_3_Level) {
    // Valid JSON includes 3 levels per node (Object/Object Instance and resource).
    // This function verifies that parsing fails in case it doesn't
    tr_debug("%s", __PRETTY_FUNCTION__);
    const std::string json_string = INVALID_JSON_NOT_3_LEVEL_1;
    M2MObjectList m2m_object_list;
    mbl::RBM2MObjectList rbm2m_object_list;
    mbl::ResourceDefinitionParser resource_parser;
    ASSERT_TRUE(resource_parser.build_object_list(json_string, m2m_object_list, rbm2m_object_list) == mbl::Error::CCRBInvalidJson);

    const std::string json_string_2 = INVALID_JSON_NOT_3_LEVEL_2;
    ASSERT_TRUE(resource_parser.build_object_list(json_string_2, m2m_object_list, rbm2m_object_list) == mbl::Error::CCRBInvalidJson);
}

TEST(JsonTest_Negative, Invalid_Json_Missing_Observable) {
    // In a valid JSON - "observable" entry is mandatory in Dynamic resource
    // This function verifies that parsing fails in case it doesn't includes it.
    tr_debug("%s", __PRETTY_FUNCTION__);
    const std::string json_string = INVALID_JSON_MISSING_OBSERVABLE_ENTRY;
    M2MObjectList m2m_object_list;
    mbl::RBM2MObjectList rbm2m_object_list;
    mbl::ResourceDefinitionParser resource_parser;
    ASSERT_TRUE(resource_parser.build_object_list(json_string, m2m_object_list, rbm2m_object_list) == mbl::Error::CCRBInvalidJson);
}

TEST(JsonTest_Negative, Illegal_resource_mode) {
    // Valid JSON includes either "dynamic" or "static" modes.
    // This function verifies that parsing fails in case of invalid mode.
    tr_debug("%s", __PRETTY_FUNCTION__);
    const std::string json_string = INVALID_JSON_ILLEGAL_RESOURCE_MODE;
    M2MObjectList m2m_object_list;
    mbl::RBM2MObjectList rbm2m_object_list;
    mbl::ResourceDefinitionParser resource_parser;
    ASSERT_TRUE(resource_parser.build_object_list(json_string, m2m_object_list, rbm2m_object_list) == mbl::Error::CCRBInvalidJson);
}

TEST(JsonTest_Negative, Illegal_resource_operation) { 
    // Valid JSON includes either "put" / "get" / "post" and "delete" as allowed operation
    // This function verifies that parsing fails in case of invalid operation.
    tr_debug("%s", __PRETTY_FUNCTION__);
    const std::string json_string = INVALID_JSON_ILLEGAL_RESOURCE_OPERATION;
    M2MObjectList m2m_object_list;
    mbl::RBM2MObjectList rbm2m_object_list;
    mbl::ResourceDefinitionParser resource_parser;
    ASSERT_TRUE(resource_parser.build_object_list(json_string, m2m_object_list, rbm2m_object_list) == mbl::Error::CCRBInvalidJson);
}

TEST(JsonTest_Negative, Unsupported_resource_type) {
    // Valid JSON includes either "string" or "integer" resource types.
    // This function verifies that parsing fails in case of invalid resource type.
    tr_debug("%s", __PRETTY_FUNCTION__);
    const std::string json_string = INVALID_JSON_UNSUPPORTED_RESOURCE_TYPE;
    M2MObjectList m2m_object_list;
    mbl::RBM2MObjectList rbm2m_object_list;
    mbl::ResourceDefinitionParser resource_parser;
    ASSERT_TRUE(resource_parser.build_object_list(json_string, m2m_object_list, rbm2m_object_list) == mbl::Error::CCRBInvalidJson);
}

TEST(JsonTest_Negative, Two_same_resource_names) {
    // Check that parsing of invalid JSON string with two same resource names fails
    tr_debug("%s", __PRETTY_FUNCTION__);
    const std::string json_string = INVALID_JSON_TWO_SAME_RESOURCE_NAMES;
    M2MObjectList m2m_object_list;
    mbl::RBM2MObjectList rbm2m_object_list;
    mbl::ResourceDefinitionParser resource_parser;
    ASSERT_TRUE(resource_parser.build_object_list(json_string, m2m_object_list, rbm2m_object_list) == mbl::Error::CCRBInvalidJson);
}

TEST(JsonTest_Negative, Two_same_object_instances) {
    // Check that parsing of invalid JSON string with two same object instance ids fails
    tr_debug("%s", __PRETTY_FUNCTION__);
    const std::string json_string = INVALID_JSON_TWO_SAME_OBJECT_INSTANCES;
    M2MObjectList m2m_object_list;
    mbl::RBM2MObjectList rbm2m_object_list;
    mbl::ResourceDefinitionParser resource_parser;
    ASSERT_TRUE(resource_parser.build_object_list(json_string, m2m_object_list, rbm2m_object_list) == mbl::Error::CCRBInvalidJson);
}

TEST(JsonTest_Negative, Two_same_objects) {
    // Check that parsing of invalid JSON string with two same object names fails
    tr_debug("%s", __PRETTY_FUNCTION__);
    const std::string json_string = INVALID_JSON_TWO_SAME_OBJECT;
    M2MObjectList m2m_object_list;
    mbl::RBM2MObjectList rbm2m_object_list;
    mbl::ResourceDefinitionParser resource_parser;
    ASSERT_TRUE(resource_parser.build_object_list(json_string, m2m_object_list, rbm2m_object_list) == mbl::Error::CCRBInvalidJson);
}
