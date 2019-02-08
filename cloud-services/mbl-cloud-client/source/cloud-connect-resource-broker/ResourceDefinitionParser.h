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


#ifndef ResourceDefinitionParser_h_
#define ResourceDefinitionParser_h_

#include "M2MResourceObjects.h"
#include "MblError.h"
#include "mbed-client/m2minterface.h"
#include <string>

namespace Json
{
    class Value;
};

namespace mbl {

/*

JSON Resource Definition:

Valid Static Resource example:

"{
    "1" : {
        "11" : { 
            "111" : { 
                "mode" : "static", 
                "resource_type" : "reset_button", 
                "type" : "string", 
                "value": "string_val", 
                "operations" : ["get"], 
                "multiple_instance" : true
            } 
        } 
    } 
}"

Valid Dynamic Resource example:
"{
    "1" : {
        "11" : {
            "111" : {
                "mode" : "dynamic", 
                "resource_type" : "reset_button", 
                "type" : "integer", 
                "value": 123456, 
                "operations" : ["get", "put", "post", "delete"], 
                "multiple_instance" : true,
                "observable" : true
            }
        }
    }
}"

In the above examples:
"1" - Object name
"11" - Object Instance Id
"111" - Resource Name

Notes:
* Each Object must have at least one Object Instance with different ids
* Each Object Instance must have at least one Resource with different names.
* Object Instance Id should be a number smaller than UINT16_MAX
* Static resource must have "get" operation and nothing else
* "type" entry is mandatory and can be either "string" or "integer"
* Resource string value length should be smaller than UINT8_MAX
* Resource integer value should be smaller than UINT8_MAX
* "resource_type" entry is optional
* "multiple_instance" and "observable" must have boolean value (e.g. true or false)
* "mode" entry is mandatory and can either be "dynamic" or "static"
* "observable" entry is mandatory in case of Dynamic resource, and it is not allowed to appear in case of Static resource

*/

class ResourceDefinitionParser {

public:

    ResourceDefinitionParser();
    virtual ~ResourceDefinitionParser();

    /**
     * @brief Build m2m2 objects / object instances and resources based on input JSON string. 
     * In case of an error - delete already created rmm2m and m2m objects / object instances and resources
     * (m2m_object_list and rbm2m_object_list will be empty).
     * @param json_string  - Input JSON string
     * @param m2m_object_list - Output M2M object list.
     * @param rbm2m_object_list - Output Resource Builder m2m object list.
     * @return MblError -
     *      Error::None - If function succeeded
     *      Error::CCRBInvalidJson - I case of invalid JSON (e.g. Invalid JSON structure or invalid M2M content such as missing mandatory entries)
     *      Error::CCRBCreateM2MObjFailed - If create of M2M object/object instance/resource failed
     */
    MblError build_object_list(
        const std::string &json_string,
        M2MObjectList &m2m_object_list,
        RBM2MObjectList &rbm2m_object_list);

private:

    /**
     * @brief Parse JSON value of an object.
     * Create M2MObject using static M2MInterfaceFactory
     * Call parse_object_instance() API to parse all nested JSON object instances
     * @param object_name - Object name
     * @param json_value_object - JSON value of an object
     * @param m2m_object_list - Hold created M2MObjects
     * @param rbm2m_object_list - Hold created BRM2MObjects 
     * @return MblError -
     *      Error::None - If function succeeded
     *      Error::CCRBInvalidJson - I case of invalid JSON (e.g. Invalid JSON structure or invalid M2M content such as missing mandatory entries)
     *      Error::CCRBCreateM2MObjFailed - If create of M2M object/object instance/resource failed
     */
    MblError parse_object(
        const std::string &object_name,
        Json::Value &json_value_object,
        M2MObjectList &m2m_object_list,
        RBM2MObjectList &rbm2m_object_list);

    /**
     * @brief Parse JSON value of an object instance.
     * Create M2MObjectInstance using M2MObject
     * Call parse_resource() API to parse all nested JSON resources
     * @param object_instance_id - Object instance ID
     * @param json_value_object_instance - JSON value of an object instance
     * @param m2m_object - M2MObject used to create M2MObjectInstance
     * @param rbm2m_object - RBM2MObject used to create RBM2MObjectInstance
     * @return MblError -
     *      Error::None - If function succeeded
     *      Error::CCRBInvalidJson - I case of invalid JSON (e.g. Invalid JSON structure or invalid M2M content such as missing mandatory entries)
     *      Error::CCRBCreateM2MObjFailed - If create of M2M object/object instance/resource failed
     */
    MblError parse_object_instance(
        int object_instance_id,
        Json::Value &json_value_object_instance,
        M2MObject *m2m_object,
        RBM2MObject * rbm2m_object);

    /**
     * @brief Parse JSON value of a resource.
     * Call CreateM2MResource() API to create M2MResource
     * @param resource_name - Resource name
     * @param json_value_resource - JSON value of a single resource
     * @param m2m_object_instance - M2MObjectInstance used to create M2MResource
     * @param rbm2m_object_instance - RBM2MObjectInstance used to create RBM2MResource
     * @return MblError -
     *      Error::None - If function succeeded
     *      Error::CCRBInvalidJson - I case of invalid JSON (e.g. Invalid JSON structure or invalid M2M content such as missing mandatory entries)
     *      Error::CCRBCreateM2MObjFailed - If create of M2M object/object instance/resource failed
     */
    MblError parse_resource(
        const std::string &resource_name,
        Json::Value &json_value_resource,
        M2MObjectInstance* m2m_object_instance,
        RBM2MObjectInstance * rbm2m_object_instance);

    /**
     * @brief Create M2MResource using M2MObjectInstance
     * @param m2m_object_instance - M2MObjectInstance used to create M2MResource
     * @param rbm2m_object_instance - RBM2MObjectInstance used to create RBM2MResource
     * @param resource_name - Resource name
     * @param resource_mode  - Resource mode (static / dynamic)
     * @param resource_res_type - Resource descritpion type (e.g. "button")
     * @param resource_type - Resource type (e.g. integer / string)
     * @param resource_value - Resource stringefied value
     * @param resource_multiple_instance - Resource support for multiple instances (= true if yes)
     * @param resource_observable  - Resource observable (= true if yes)
     * @param operation_mask - Operation mask used to map to m2m operation
     * @return MblError -
     *      Error::None - If function succeeded
     *      Error::CCRBCreateM2MObjFailed - If create of M2M object/object instance/resource failed
     */
    MblError create_resources(
        M2MObjectInstance *m2m_object_instance,
        RBM2MObjectInstance *rbm2m_object_instance,
        const std::string &resource_name,
        const std::string &resource_mode,
        const std::string &resource_res_type,
        const std::string &resource_type, 
        const std::string &resource_value, 
        bool resource_multiple_instance, 
        bool resource_observable,
        uint8_t operation_mask);
};

} // namespace mbl

#endif // ResourceDefinitionParser_h_
