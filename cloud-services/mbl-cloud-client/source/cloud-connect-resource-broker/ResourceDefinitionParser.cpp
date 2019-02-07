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

#include "ResourceDefinitionParser.h"
#include "mbed-trace/mbed_trace.h"
#include "mbed-client/m2mresource.h"
#include "mbed-client/m2minterfacefactory.h"

#include <json/json.h>
#include <json/reader.h>
#include <cassert>

#define TRACE_GROUP "ccrb-resdefparser"

#define JSON_RESOURCE_MODE                  "mode"
#define JSON_RESOURCE_MODE_STATIC           "static"
#define JSON_RESOURCE_MODE_DYNAMIC          "dynamic"
#define JSON_RESOURCE_TYPE                  "type"
#define JSON_RESOURCE_TYPE_INTEGER          "integer"
#define JSON_RESOURCE_TYPE_STRING           "string"
#define JSON_RESOURCE_VALUE                 "value"
#define JSON_RESOURCE_RES_TYPE              "resource_type"
#define JSON_RESOURCE_OPERATIONS            "operations"
#define JSON_RESOURCE_OPERATION_PUT         "put"
#define JSON_RESOURCE_OPERATION_GET         "get"
#define JSON_RESOURCE_OPERATION_POST        "post"
#define JSON_RESOURCE_OPERATION_DELETE      "delete"
#define JSON_RESOURCE_MULTIPLE_INSTANCE     "multiple_instance"
#define JSON_RESOURCE_OBSERVABLE            "observable"

#define OP_MASK_NONE_ALLOWED        0
#define OP_MASK_GET_ALLOWED         1
#define OP_MASK_PUT_ALLOWED         2
#define OP_MASK_POST_ALLOWED        4
#define OP_MASK_DELETE_ALLOWED      8

static const M2MBase::Operation mask_to_operation[] = {
    M2MBase::NOT_ALLOWED,                   //0
    M2MBase::GET_ALLOWED,                   //1
    M2MBase::PUT_ALLOWED,                   //2
    M2MBase::GET_PUT_ALLOWED,               //3
    M2MBase::POST_ALLOWED,                  //4
    M2MBase::GET_POST_ALLOWED,              //5
    M2MBase::PUT_POST_ALLOWED,              //6
    M2MBase::GET_PUT_POST_ALLOWED,          //7
    M2MBase::DELETE_ALLOWED,                //8
    M2MBase::GET_DELETE_ALLOWED,            //9
    M2MBase::PUT_DELETE_ALLOWED,            //10
    M2MBase::GET_PUT_DELETE_ALLOWED,        //11
    M2MBase::POST_DELETE_ALLOWED,           //12
    M2MBase::GET_POST_DELETE_ALLOWED,       //13
    M2MBase::PUT_POST_DELETE_ALLOWED,       //14
    M2MBase::GET_PUT_POST_DELETE_ALLOWED    //15
} ;

namespace mbl {

// Helper functions:
static M2MResourceInstance::ResourceType get_m2m_resource_type(const std::string &resource_type)
{
    if(resource_type == JSON_RESOURCE_TYPE_INTEGER) {
        return M2MResourceInstance::INTEGER;
    }
    if(resource_type == JSON_RESOURCE_TYPE_STRING) {
        return M2MResourceInstance::STRING;
    }
    // We already made validity checks on "resource_type" before calling this function so we shouldn't be here
    tr_warning("%s - Invalid resource type: %s", __PRETTY_FUNCTION__, resource_type.c_str());
    return M2MResourceInstance::OPAQUE;
}

static MblError get_m2m_resource_operation(uint32_t operation_mask, M2MBase::Operation *operation)
{
    // Range check
    if(operation_mask >= (sizeof(mask_to_operation) / sizeof(M2MBase::Operation))) {
        tr_error("%s - Invalid operaion mask: %d", __PRETTY_FUNCTION__, operation_mask);
        return Error::CCRBInvalidJson;
    }
    *operation = mask_to_operation[operation_mask];
    return Error::None;
}

////////////////////////////////////////////////////////////////////////////////

ResourceDefinitionParser::ResourceDefinitionParser()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

ResourceDefinitionParser::~ResourceDefinitionParser()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

MblError ResourceDefinitionParser::create_resources(
    M2MObjectInstance *m2m_object_instance,
    RBM2MObjectInstance *rbm2m_object_instance,
    const std::string &resource_name,
    const std::string &resource_mode,
    const std::string &resource_res_type,
    const std::string &resource_type,
    const std::string &resource_value,
    bool resource_multiple_instance,
    bool resource_observable,
    uint32_t operation_mask)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    M2MBase::Operation m2m_operation;
    M2MResourceInstance::ResourceType m2m_res_type = get_m2m_resource_type(resource_type);
    MblError retval = get_m2m_resource_operation(operation_mask, &m2m_operation);
    if(Error::None != retval) {
        tr_error("%s - get_m2m_resource_operation failed", __PRETTY_FUNCTION__);
        return retval;
    }

    M2MResource* m2m_resource = nullptr;
    M2MBase::Mode m2m_mode = M2MBase::Dynamic;
    tr_info("Create %s resource: %s", resource_mode.c_str(), resource_name.c_str());
    if (resource_mode == JSON_RESOURCE_MODE_STATIC) {
        m2m_mode = M2MBase::Static;
        const uint8_t value_length = static_cast<uint8_t>(resource_value.length()); // During the JSON parsing we verify valid length so we are safe
        m2m_resource = m2m_object_instance->create_static_resource(
            resource_name.c_str(),
            resource_res_type.c_str(),
            m2m_res_type,
            (const unsigned char*)resource_value.c_str(),
            value_length,
            resource_multiple_instance);
    } else {
        m2m_resource = m2m_object_instance->create_dynamic_resource(
            resource_name.c_str(),
            resource_res_type.c_str(),
            m2m_res_type,
            resource_observable,
            resource_multiple_instance);
    }
    if(nullptr == m2m_resource) {
        tr_error("%s - Create %s m2m_resource: %s failed", __PRETTY_FUNCTION__, resource_mode.c_str(), resource_name.c_str());
        return Error::CCRBCreateM2MObjFailed;
    }
    tr_debug("Set M2MResource operation to %d", m2m_operation);
    m2m_resource->set_operation(m2m_operation); // Set allowed operations for accessing the resource

    // Create rbm2m resource and add it to rbm2m_object_instance's map
    auto rbm2m_resource = rbm2m_object_instance->create_resource(
        resource_name,
        m2m_mode,
        resource_multiple_instance,
        m2m_operation,
        resource_observable,
        resource_res_type,
        m2m_res_type,
        resource_value);
    if(nullptr == rbm2m_resource) {
        tr_error("%s - Create rbm2m_resource: %s failed", __PRETTY_FUNCTION__, resource_name.c_str());
        return Error::CCRBCreateM2MObjFailed;
    }
    rbm2m_resource->set_m2m_resource(m2m_resource);

    return Error::None;
}

MblError ResourceDefinitionParser::parse_resource(
    const std::string &resource_name,
    Json::Value &json_value_resource,
    M2MObjectInstance* m2m_object_instance,
    RBM2MObjectInstance * rbm2m_object_instance)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    tr_debug("--------");
    tr_debug("resource_name: %s", resource_name.c_str());
    //Leave the next debug line commented. Uncomment in case of heavy debuging
    //tr_debug("value: %s", json_value_resource.toStyledString().c_str());

    if(json_value_resource.empty()) {
        //Error: Invalid JSON, we support only JSONs with 3 levels (Obj/ObjInstance/Resource)
        tr_error("%s - Invalid JSON. Resource is empty.", __PRETTY_FUNCTION__);
        return Error::CCRBInvalidJson;
    }

    std::string resource_mode;
    std::string resource_value;
    std::string resource_type;
    std::string resource_res_type;
    std::string resource_operation;
    uint32_t operation_mask = OP_MASK_NONE_ALLOWED;
    bool found_res_multiple_instance = false;
    bool resource_multiple_instance = false;
    bool found_res_observable = false;
    bool resource_observable = false;

    for(auto itr = json_value_resource.begin() ; itr != json_value_resource.end() ; itr++) 
    {
        Json::Value res = *itr;
        Json::Value res_name = itr.key();

        // Verify all mandatory fields are found and found only once 
        // (same entry will override the previous one and hence is threated as an error)
        if(resource_mode.empty() && res_name.asString() == JSON_RESOURCE_MODE){
            resource_mode = res.asString();
            if(resource_mode != JSON_RESOURCE_MODE_STATIC && resource_mode != JSON_RESOURCE_MODE_DYNAMIC) {
                tr_error("%s - Invalid JSON. Unknown mode: %s.", __PRETTY_FUNCTION__, resource_mode.c_str());
                return Error::CCRBInvalidJson;
            }
            tr_debug("mode: %s", resource_mode.c_str());
        } else if (resource_value.empty() && res_name.asString() == JSON_RESOURCE_VALUE){
            resource_value = res.asString();
            if (resource_value.length() > UINT8_MAX) {
                tr_error("%s - Invalid JSON. Allowed value length should be between 0 and %d", __PRETTY_FUNCTION__, UINT8_MAX);
                return Error::CCRBInvalidJson;
            }
            tr_debug("value: %s", resource_value.c_str());
        } else if (resource_res_type.empty() && res_name.asString() == JSON_RESOURCE_RES_TYPE){
            resource_res_type = res.asString();
            tr_debug("resource type: %s", resource_res_type.c_str());
        } else if (resource_type.empty() && res_name.asString() == JSON_RESOURCE_TYPE){
            resource_type = res.asString();
            //TODO: currently supporting only integer and string types. Need to support all types.
            if(resource_type != JSON_RESOURCE_TYPE_INTEGER && resource_type != JSON_RESOURCE_TYPE_STRING) {
                tr_error("%s - Invalid JSON. Resource type not supported: %s.", __PRETTY_FUNCTION__, resource_type.c_str());
                return Error::CCRBInvalidJson;
            }
            tr_debug("type: %s", resource_type.c_str());
        } else if ( (operation_mask == OP_MASK_NONE_ALLOWED) && (res_name.asString() == JSON_RESOURCE_OPERATIONS) ) {
            if(!res.isArray()) {
                tr_error("%s - Invalid JSON. %s field is expeted to be an array.", __PRETTY_FUNCTION__, JSON_RESOURCE_OPERATIONS);
                return Error::CCRBInvalidJson;
            }

            tr_debug("Operations: ");
            // In case of operation we are going to be less strict and allow several same entries
            for (const auto& op : res) {
                if(!op.isString()) {
                    tr_error("%s - Invalid JSON. %s array entry not string element.", __PRETTY_FUNCTION__, JSON_RESOURCE_OPERATIONS);
                    return Error::CCRBInvalidJson;
                }
                resource_operation = op.asString().c_str();
                if (resource_operation == JSON_RESOURCE_OPERATION_PUT) {
                    operation_mask |= OP_MASK_PUT_ALLOWED;
                    tr_debug("%s, ", JSON_RESOURCE_OPERATION_PUT);
                } else if (resource_operation == JSON_RESOURCE_OPERATION_GET) {
                    operation_mask |= OP_MASK_GET_ALLOWED;
                    tr_debug("%s, ", JSON_RESOURCE_OPERATION_GET);
                } else if (resource_operation == JSON_RESOURCE_OPERATION_POST) {
                    operation_mask |= OP_MASK_POST_ALLOWED;
                    tr_debug("%s, ", JSON_RESOURCE_OPERATION_POST);
                } else if (resource_operation == JSON_RESOURCE_OPERATION_DELETE) {
                    operation_mask |= OP_MASK_DELETE_ALLOWED;
                    tr_debug("%s, ", JSON_RESOURCE_OPERATION_DELETE);
                } else {
                    tr_error("%s - Invalid JSON. Unknown operation: %s.", __PRETTY_FUNCTION__, resource_operation.c_str());
                    return Error::CCRBInvalidJson;
                }
            }
        } else if (!found_res_multiple_instance && res_name.asString() == JSON_RESOURCE_MULTIPLE_INSTANCE){
            resource_multiple_instance = res.asBool(); // Will throw exception if not bool!
            tr_debug("multiple instance: %s", res.asString().c_str());
            found_res_multiple_instance = true;
        } else if (!found_res_observable && res_name.asString() == JSON_RESOURCE_OBSERVABLE){
            resource_observable = res.asBool(); // Will throw exception if not bool!
            tr_debug("observable: %s [%d]", res.asString().c_str(), resource_observable);
            found_res_observable = true;
        } else {
            tr_error("%s - Invalid JSON. Entry %s was either found twice or is unknown entry.", __PRETTY_FUNCTION__, res_name.asString().c_str());
            return Error::CCRBInvalidJson;
        }
    }

    // Logic checks on resource_mode
    if (resource_mode == JSON_RESOURCE_MODE_STATIC) {
        if(found_res_observable) {
            tr_error("%s - Invalid JSON. Observable entry should only be used in dynamic resource", __PRETTY_FUNCTION__);
            return Error::CCRBInvalidJson;
        }
        if(operation_mask != OP_MASK_GET_ALLOWED) {
            tr_error("%s - Invalid JSON. Static resource operation must be: get", __PRETTY_FUNCTION__);
            return Error::CCRBInvalidJson;
        }
    } else {
        if(!found_res_observable) {
            tr_error("%s - Invalid JSON. %s entry is mandatory for dynamic resource", __PRETTY_FUNCTION__, JSON_RESOURCE_OBSERVABLE);
            return Error::CCRBInvalidJson;
        }
    }

    create_resources(m2m_object_instance, rbm2m_object_instance, resource_name, resource_mode, resource_res_type, resource_type,
        resource_value, resource_multiple_instance, resource_observable, operation_mask);
    return Error::None;
}

MblError ResourceDefinitionParser::parse_object_instance(
    int object_instance_id,
    Json::Value &json_value_object_instance,
    M2MObject *m2m_object,
    RBM2MObject * rbm2m_object)
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    tr_debug("--------");
    tr_debug("object instance id: %d", object_instance_id);

    if(json_value_object_instance.empty()) {
        //Error: Invalid JSON, we support only JSONs with 3 levels (Obj/ObjInstance/Resource)
        tr_error("%s - Invalid JSON. ObjectInstance is empty.", __PRETTY_FUNCTION__);
        return Error::CCRBInvalidJson;
    }

    if (object_instance_id < 0 || object_instance_id > UINT16_MAX) {
        tr_error("%s - Invalid JSON. object_instance_id allowed value should be between 0 and %d", __PRETTY_FUNCTION__, UINT16_MAX);
        return Error::CCRBInvalidJson;
    }
    uint16_t object_instance_id_uint16 = static_cast<uint16_t>(object_instance_id); // We are safe after the above check

    // Create m2m object instance
    auto m2m_object_instance = m2m_object->create_object_instance(object_instance_id_uint16);
    if(nullptr == m2m_object_instance) {
        tr_error("%s - Create m2m_object_instance id: %d failed", __PRETTY_FUNCTION__, object_instance_id);
        return Error::CCRBCreateM2MObjFailed;
    }
    // Create rbm2m object instance and add it to rbm2m_object's map
    auto rbm2m_object_instance = rbm2m_object->create_object_instance(object_instance_id_uint16);
    if(nullptr == rbm2m_object_instance){
        tr_error("%s - Create rbm2m_object_instance: %d failed", __PRETTY_FUNCTION__, object_instance_id);
        return Error::CCRBCreateM2MObjFailed;
    }
    tr_debug("Created rbm2m_object_instance: %d [%p]", object_instance_id, rbm2m_object_instance);

    rbm2m_object_instance->set_m2m__object_instance(m2m_object_instance);

    for(auto itr = json_value_object_instance.begin() ; itr != json_value_object_instance.end() ; itr++) 
    {
        Json::Value json_value_resource = *itr;
        Json::Value resource_name = itr.key();
        if(parse_resource(resource_name.asString(), json_value_resource, m2m_object_instance, rbm2m_object_instance) != Error::None){
            tr_error("%s - parse_resource failed.", __PRETTY_FUNCTION__);
            return Error::CCRBInvalidJson;
        }
    }
    
    return Error::None;
}

MblError ResourceDefinitionParser::parse_object(
    const std::string &object_name,
    Json::Value &json_value_object,
    M2MObjectList &m2m_object_list,
    RBM2MObjectList &rbm2m_object_list)
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    tr_debug("----------------");
    tr_debug("object_name: %s", object_name.c_str());

    if(json_value_object.empty()) {
        //Error: Invalid JSON, we support only JSONs with 3 levels (Obj/Object Instance/Resource)
        tr_error("%s - Invalid JSON. ObjectID is empty.", __PRETTY_FUNCTION__);
        return Error::CCRBInvalidJson;
    }
    // Create m2m object and push it to list
    auto m2m_object = M2MInterfaceFactory::create_object(object_name.c_str());
    if(nullptr == m2m_object) {
        tr_error("%s - Create m2m_object: %s failed", __PRETTY_FUNCTION__, object_name.c_str());
        return Error::CCRBCreateM2MObjFailed;
    }
    tr_debug("Created m2m_object: %s [%p]", m2m_object->name(), m2m_object);
    m2m_object_list.push_back(m2m_object);
    // Create rbm2m object and add it to rbm2m_object_list
    auto rbm2m_object = rbm2m_object_list.create_object(object_name);
    if(nullptr == rbm2m_object){
        tr_error("%s - Create rbm2m_object: %s failed", __PRETTY_FUNCTION__, object_name.c_str());
        return Error::CCRBCreateM2MObjFailed;
    }
    tr_debug("Created rbm2m_object: %s [%p]", m2m_object->name(), rbm2m_object);

    rbm2m_object->set_m2m_object(m2m_object);

    for(auto itr = json_value_object.begin() ; itr != json_value_object.end() ; itr++) 
    {
        Json::Value json_value_object_instance = *itr;
        Json::Value object_instance_id = itr.key();
        if(parse_object_instance(std::stoi(object_instance_id.asString()), json_value_object_instance, m2m_object, rbm2m_object) != Error::None){
            tr_error("%s - parse_object_instance failed.", __PRETTY_FUNCTION__);
            return Error::CCRBInvalidJson;
        }
    }

    return Error::None;
}

MblError ResourceDefinitionParser::build_object_list(
    const std::string &json_string,
    M2MObjectList &m2m_object_list,
    RBM2MObjectList &rbm2m_object_list)
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    MblError retval = Error::None;
    // We must catch exception to avoid crashes when application send invalid / corrupted JSON
    try {
        Json::CharReaderBuilder builder;
        Json::Value json_settings;

        // Set strict mode
        builder.strictMode(&json_settings); // Init json_settings with strict mode values
        builder.settings_ = json_settings;
        Json::CharReader * reader = builder.newCharReader();
        Json::Value root;
        std::string errors;

        // Parse
        bool parsing_successful = reader->parse(json_string.c_str(), json_string.c_str() + json_string.size(), &root, &errors);
        delete reader;
        if (!parsing_successful) {
            tr_error("%s - parsing Json string failed with errors: %s.", __PRETTY_FUNCTION__, errors.c_str());
            return Error::CCRBInvalidJson;
        }
        if(root.empty()) {
            tr_error("%s - Invalid JSON. Root is empty.", __PRETTY_FUNCTION__);
            return Error::CCRBInvalidJson;
        }
        // Parse all objects:
        for(auto itr = root.begin() ; itr != root.end() ; itr++) 
        {
            Json::Value json_value_object = *itr;
            Json::Value object_name = itr.key();
            retval = parse_object(object_name.asString(), json_value_object, m2m_object_list, rbm2m_object_list);
            if(retval != Error::None) {
                tr_error("%s - parse_object failed with error %s", __PRETTY_FUNCTION__, MblError_to_str(retval));
                break;
            }
        }
    }
    catch (const std::runtime_error& e) {
        tr_error("%s - BuildResourceList failed with runtime_error exception msg: %s.", __PRETTY_FUNCTION__, e.what ());
        retval = Error::CCRBInvalidJson;
    }
    catch (std::exception &e) {
        tr_error("%s - BuildResourceList failed with std::exception exception msg: %s.", __PRETTY_FUNCTION__, e.what ());
        retval = Error::CCRBInvalidJson;
    } 
    catch (...) {
        tr_error("%s - BuildResourceList failed with exception.", __PRETTY_FUNCTION__);
        retval = Error::CCRBInvalidJson;
    }

    //Free allocated memory for object instances in case error occured
    if(retval != Error::None && !m2m_object_list.empty()) {
        M2MObject* m2m_object = nullptr;
        for (auto &itr : m2m_object_list) {
            m2m_object = itr;
            tr_debug("Deleting m2m_object: %s [%p]", m2m_object->name(), m2m_object);
            delete m2m_object; // This will delete all created object instances and all resources that belongs to it
        }
        m2m_object_list.clear();
    }
    return retval;
}

} // namespace mbl
