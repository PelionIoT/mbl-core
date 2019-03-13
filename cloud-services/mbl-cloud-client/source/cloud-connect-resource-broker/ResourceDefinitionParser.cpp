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
#include "CloudConnectTrace.h"
#include "m2minterfacefactory.h"
#include "m2mresource.h"

#include <cassert>
#include <cinttypes>
#include <json/json.h>
#include <json/reader.h>

#define TRACE_GROUP "ccrb-resource-parser"

#define JSON_RESOURCE_MODE "mode"
#define JSON_RESOURCE_MODE_STATIC "static"
#define JSON_RESOURCE_MODE_DYNAMIC "dynamic"
#define JSON_RESOURCE_TYPE "type"
#define JSON_RESOURCE_TYPE_INTEGER "integer"
#define JSON_RESOURCE_TYPE_STRING "string"
#define JSON_RESOURCE_VALUE "value"
#define JSON_RESOURCE_RES_TYPE "resource_type"
#define JSON_RESOURCE_OPERATIONS "operations"
#define JSON_RESOURCE_OPERATION_PUT "put"
#define JSON_RESOURCE_OPERATION_GET "get"
#define JSON_RESOURCE_OPERATION_POST "post"
#define JSON_RESOURCE_OPERATION_DELETE "delete"
#define JSON_RESOURCE_MULTIPLE_INSTANCE "multiple_instance"
#define JSON_RESOURCE_OBSERVABLE "observable"

#define OP_MASK_NONE_ALLOWED 0
#define OP_MASK_GET_ALLOWED 1
#define OP_MASK_PUT_ALLOWED 2
#define OP_MASK_POST_ALLOWED 4
#define OP_MASK_DELETE_ALLOWED 8

namespace mbl
{

// Init static member operation_map_
const ResourceDefinitionParser::OperationMap ResourceDefinitionParser::operation_ = {
    {OP_MASK_NONE_ALLOWED, M2MBase::NOT_ALLOWED},                            // 0
    {OP_MASK_GET_ALLOWED, M2MBase::GET_ALLOWED},                             // 1
    {OP_MASK_PUT_ALLOWED, M2MBase::PUT_ALLOWED},                             // 2
    {OP_MASK_GET_ALLOWED | OP_MASK_PUT_ALLOWED, M2MBase::GET_PUT_ALLOWED},   // 3
    {OP_MASK_POST_ALLOWED, M2MBase::POST_ALLOWED},                           // 4
    {OP_MASK_GET_ALLOWED | OP_MASK_POST_ALLOWED, M2MBase::GET_POST_ALLOWED}, // 5
    {OP_MASK_PUT_ALLOWED | OP_MASK_POST_ALLOWED, M2MBase::PUT_POST_ALLOWED}, // 6
    {OP_MASK_GET_ALLOWED | OP_MASK_PUT_ALLOWED | OP_MASK_POST_ALLOWED,
     M2MBase::GET_PUT_POST_ALLOWED},                                             // 7
    {OP_MASK_DELETE_ALLOWED, M2MBase::DELETE_ALLOWED},                           // 8
    {OP_MASK_GET_ALLOWED | OP_MASK_DELETE_ALLOWED, M2MBase::GET_DELETE_ALLOWED}, // 9
    {OP_MASK_PUT_ALLOWED | OP_MASK_DELETE_ALLOWED, M2MBase::PUT_DELETE_ALLOWED}, // 10
    {OP_MASK_GET_ALLOWED | OP_MASK_PUT_ALLOWED | OP_MASK_DELETE_ALLOWED,
     M2MBase::GET_PUT_DELETE_ALLOWED},                                             // 11
    {OP_MASK_POST_ALLOWED | OP_MASK_DELETE_ALLOWED, M2MBase::POST_DELETE_ALLOWED}, // 12
    {OP_MASK_GET_ALLOWED | OP_MASK_POST_ALLOWED | OP_MASK_DELETE_ALLOWED,
     M2MBase::GET_POST_DELETE_ALLOWED}, // 13
    {OP_MASK_PUT_ALLOWED | OP_MASK_POST_ALLOWED | OP_MASK_DELETE_ALLOWED,
     M2MBase::PUT_POST_DELETE_ALLOWED}, // 14
    {OP_MASK_GET_ALLOWED | OP_MASK_PUT_ALLOWED | OP_MASK_POST_ALLOWED | OP_MASK_DELETE_ALLOWED,
     M2MBase::GET_PUT_POST_DELETE_ALLOWED} // 15
};

M2MResourceInstance::ResourceType
ResourceDefinitionParser::get_m2m_resource_type(const std::string& resource_type)
{
    TR_DEBUG_ENTER;

    if (resource_type == JSON_RESOURCE_TYPE_INTEGER) {
        return M2MResourceInstance::INTEGER;
    }
    if (resource_type == JSON_RESOURCE_TYPE_STRING) {
        return M2MResourceInstance::STRING;
    }
    // We already made validity checks on "resource_type" before calling this
    // function so we shouldn't be here
    assert(0); // Shouldn't be here!
}

MblError ResourceDefinitionParser::get_m2m_resource_operation(uint8_t operation_mask,
                                                              M2MBase::Operation& operation)
{
    TR_DEBUG_ENTER;

    // Verify operation mast is valid
    auto itr = operation_.find(operation_mask);
    if (itr == operation_.end()) {
        TR_ERR("Invalid operaion mask: %" PRId8, operation_mask);
        return Error::CCRBInvalidJson;
    }
    operation = itr->second;
    return Error::None;
}

MblError ResourceDefinitionParser::create_resources(M2MObjectInstance* m2m_object_instance,
                                                    const std::string& resource_name,
                                                    const std::string& resource_mode,
                                                    const std::string& resource_res_type,
                                                    const std::string& resource_type,
                                                    const std::string& resource_value,
                                                    bool resource_multiple_instance,
                                                    bool resource_observable,
                                                    uint8_t operation_mask)
{
    TR_DEBUG_ENTER;

    M2MBase::Operation m2m_operation;
    M2MResourceInstance::ResourceType m2m_res_type = get_m2m_resource_type(resource_type);
    MblError retval = get_m2m_resource_operation(operation_mask, m2m_operation);
    if (Error::None != retval) {
        TR_ERR("get_m2m_resource_operation failed");
        return retval;
    }

    M2MResource* m2m_resource = nullptr;
    TR_INFO("Create %s resource: %s", resource_mode.c_str(), resource_name.c_str());
    if (resource_mode == JSON_RESOURCE_MODE_STATIC) {
        m2m_resource = m2m_object_instance->create_static_resource(resource_name.c_str(),
                                                                   resource_res_type.c_str(),
                                                                   m2m_res_type,
                                                                   nullptr,
                                                                   0,
                                                                   resource_multiple_instance);
    }
    else
    {
        m2m_resource = m2m_object_instance->create_dynamic_resource(resource_name.c_str(),
                                                                    resource_res_type.c_str(),
                                                                    m2m_res_type,
                                                                    resource_observable,
                                                                    resource_multiple_instance);
    }
    if (nullptr == m2m_resource) {
        TR_ERR("Create %s m2m_resource: %s failed", resource_mode.c_str(), resource_name.c_str());
        return Error::CCRBCreateM2MObjFailed;
    }
    // Set value for the created static / dynamic resource
    if (!resource_value.empty()) {
        // During the JSON parsing we verify valid length so we are safe:
        const uint8_t value_length = static_cast<uint8_t>(resource_value.length());
        std::vector<uint8_t> value(resource_value.begin(), resource_value.end());
        if (!m2m_resource->set_value(value.data(), value_length)) {
            TR_ERR("Set value of resource: %s failed", resource_name.c_str());
            return Error::CCRBCreateM2MObjFailed;
        }
    }

    TR_DEBUG("Set M2MResource operation to %d", m2m_operation);
    m2m_resource->set_operation(m2m_operation); // Set allowed operations for accessing the resource
    return Error::None;
}

MblError ResourceDefinitionParser::parse_operation(Json::Value& resource, uint8_t* operation_mask)
{
    TR_DEBUG_ENTER;

    *operation_mask = OP_MASK_NONE_ALLOWED;
    if (!resource.isArray()) {
        TR_ERR("Invalid JSON. %s field is expeted to be an array.", JSON_RESOURCE_OPERATIONS);
        return Error::CCRBInvalidJson;
    }

    // In case of operation we are going to be less strict and allow several same
    // entries
    std::string resource_operation;
    for (const auto& op : resource) {
        if (!op.isString()) {
            TR_ERR("Invalid JSON. %s array entry not string element.", JSON_RESOURCE_OPERATIONS);
            return Error::CCRBInvalidJson;
        }
        resource_operation = op.asString().c_str();
        if (resource_operation == JSON_RESOURCE_OPERATION_PUT) {
            *operation_mask |= OP_MASK_PUT_ALLOWED;
            TR_DEBUG("%s, ", JSON_RESOURCE_OPERATION_PUT);
        }
        else if (resource_operation == JSON_RESOURCE_OPERATION_GET)
        {
            *operation_mask |= OP_MASK_GET_ALLOWED;
            TR_DEBUG("%s, ", JSON_RESOURCE_OPERATION_GET);
        }
        else if (resource_operation == JSON_RESOURCE_OPERATION_POST)
        {
            *operation_mask |= OP_MASK_POST_ALLOWED;
            TR_DEBUG("%s, ", JSON_RESOURCE_OPERATION_POST);
        }
        else if (resource_operation == JSON_RESOURCE_OPERATION_DELETE)
        {
            *operation_mask |= OP_MASK_DELETE_ALLOWED;
            TR_DEBUG("%s, ", JSON_RESOURCE_OPERATION_DELETE);
        }
        else
        {
            TR_ERR("Invalid JSON. Unknown operation: %s.", resource_operation.c_str());
            return Error::CCRBInvalidJson;
        }
    }
    return Error::None;
}

MblError ResourceDefinitionParser::parse_resource(const std::string& resource_name,
                                                  Json::Value& resource_definition,
                                                  M2MObjectInstance* m2m_object_instance)
{
    TR_DEBUG("resource_name: %s", resource_name.c_str());

    // Leave the next debug line commented. Uncomment in case of heavy debuging
    // TR_DEBUG("value: %s", resource_definition.toStyledString().c_str());

    if (resource_definition.empty()) {
        // Error: Invalid JSON, we support only JSONs with 3 levels
        // (Obj/ObjInstance/Resource)
        TR_ERR("Invalid JSON. Resource is empty.");
        return Error::CCRBInvalidJson;
    }

    std::string resource_mode;
    std::string resource_value;
    std::string resource_type;
    std::string resource_res_type;
    uint8_t operation_mask = OP_MASK_NONE_ALLOWED;
    bool found_res_multiple_instance = false;
    bool resource_multiple_instance = false;
    bool found_res_observable = false;
    bool resource_observable = false;

    for (auto itr = resource_definition.begin(); itr != resource_definition.end(); itr++) {
        Json::Value res = *itr;
        Json::Value res_name = itr.key();

        // Verify all mandatory fields are found and found only once
        // (same entry will override the previous one and hence is threated as an
        // error)
        if (resource_mode.empty() && res_name.asString() == JSON_RESOURCE_MODE) {
            resource_mode = res.asString();
            if (resource_mode != JSON_RESOURCE_MODE_STATIC &&
                resource_mode != JSON_RESOURCE_MODE_DYNAMIC)
            {
                TR_ERR("Invalid JSON. Unknown mode: %s.", resource_mode.c_str());
                return Error::CCRBInvalidJson;
            }
            TR_DEBUG("mode: %s", resource_mode.c_str());
        }
        else if (resource_value.empty() && res_name.asString() == JSON_RESOURCE_VALUE)
        {
            resource_value = res.asString();
            if (resource_value.length() > UINT8_MAX) {
                TR_ERR("Invalid JSON. Allowed value length should be between 0 and %d", UINT8_MAX);
                return Error::CCRBInvalidJson;
            }
            TR_DEBUG("value: %s", resource_value.c_str());
        }
        else if (resource_res_type.empty() && res_name.asString() == JSON_RESOURCE_RES_TYPE)
        {
            resource_res_type = res.asString();
            TR_DEBUG("resource type: %s", resource_res_type.c_str());
        }
        else if (resource_type.empty() && res_name.asString() == JSON_RESOURCE_TYPE)
        {
            resource_type = res.asString();
            // TODO: currently supporting only integer and string types. Need to
            // support all types.
            if (resource_type != JSON_RESOURCE_TYPE_INTEGER &&
                resource_type != JSON_RESOURCE_TYPE_STRING)
            {
                TR_ERR("Invalid JSON. Resource type not supported: %s", resource_type.c_str());
                return Error::CCRBInvalidJson;
            }
            TR_DEBUG("type: %s", resource_type.c_str());
        }
        else if ((operation_mask == OP_MASK_NONE_ALLOWED) &&
                 (res_name.asString() == JSON_RESOURCE_OPERATIONS))
        {
            const MblError parse_op_status = parse_operation(res, &operation_mask);
            if (Error::None != parse_op_status) {
                TR_ERR("Invalid JSON. Error parsing %s entry.", JSON_RESOURCE_OPERATIONS);
                return Error::CCRBInvalidJson;
            }
        }
        else if (!found_res_multiple_instance &&
                 res_name.asString() == JSON_RESOURCE_MULTIPLE_INSTANCE)
        {
            resource_multiple_instance = res.asBool(); // Will throw exception if not bool!
            TR_DEBUG("multiple instance: %s", res.asString().c_str());
            found_res_multiple_instance = true;
        }
        else if (!found_res_observable && res_name.asString() == JSON_RESOURCE_OBSERVABLE)
        {
            resource_observable = res.asBool(); // Will throw exception if not bool!
            TR_DEBUG("observable: %s [%d]", res.asString().c_str(), resource_observable);
            found_res_observable = true;
        }
        else
        {
            TR_ERR("Invalid JSON. Entry %s was either found twice or is unknown entry.",
                   res_name.asString().c_str());
            return Error::CCRBInvalidJson;
        }
    }

    // Logic checks on resource_mode
    if (resource_mode == JSON_RESOURCE_MODE_STATIC) {
        if (found_res_observable) {
            TR_ERR("Invalid JSON. Observable entry should only be used in dynamic resource");
            return Error::CCRBInvalidJson;
        }
        if (operation_mask != OP_MASK_GET_ALLOWED) {
            TR_ERR("Invalid JSON. Static resource operation must be: get");
            return Error::CCRBInvalidJson;
        }
    }
    else
    {
        if (!found_res_observable) {
            TR_ERR("Invalid JSON. %s entry is mandatory for dynamic resource",
                   JSON_RESOURCE_OBSERVABLE);
            return Error::CCRBInvalidJson;
        }
    }

    return create_resources(m2m_object_instance,
                            resource_name,
                            resource_mode,
                            resource_res_type,
                            resource_type,
                            resource_value,
                            resource_multiple_instance,
                            resource_observable,
                            operation_mask);
}

MblError ResourceDefinitionParser::parse_object_instance(int object_instance_id,
                                                         Json::Value& object_instance_definition,
                                                         M2MObject* m2m_object)
{
    TR_DEBUG("object instance id: %d", object_instance_id);

    if (object_instance_definition.empty()) {
        // Error: Invalid JSON, we support only JSONs with 3 levels
        // (Obj/ObjInstance/Resource)
        TR_ERR("Invalid JSON. ObjectInstance is empty.");
        return Error::CCRBInvalidJson;
    }

    if (object_instance_id < 0 || object_instance_id > UINT16_MAX) {
        TR_ERR("Invalid JSON. object_instance_id allowed value should be between 0 and %d",
               UINT16_MAX);
        return Error::CCRBInvalidJson;
    }
    uint16_t object_instance_id_uint16 =
        static_cast<uint16_t>(object_instance_id); // We are safe after the above check

    // Create m2m object instance
    auto m2m_object_instance = m2m_object->create_object_instance(object_instance_id_uint16);
    if (nullptr == m2m_object_instance) {
        TR_ERR("Create m2m_object_instance id: %d failed", object_instance_id);
        return Error::CCRBCreateM2MObjFailed;
    }

    for (auto itr = object_instance_definition.begin(); itr != object_instance_definition.end();
         itr++)
    {
        Json::Value resource_definition = *itr;
        Json::Value resource_name = itr.key();
        if (parse_resource(resource_name.asString(), resource_definition, m2m_object_instance) !=
            Error::None)
        {
            TR_ERR("parse_resource failed.");
            return Error::CCRBInvalidJson;
        }
    }

    return Error::None;
}

MblError ResourceDefinitionParser::parse_object(const std::string& object_name,
                                                Json::Value& object_definition,
                                                M2MObjectList& m2m_object_list)
{
    TR_DEBUG("object_name: %s", object_name.c_str());

    if (object_definition.empty()) {
        // Error: Invalid JSON, we support only JSONs with 3 levels
        // (Obj/Object Instance/Resource)
        TR_ERR("Invalid JSON. ObjectID is empty.");
        return Error::CCRBInvalidJson;
    }
    // Create m2m object and push it to list
    auto m2m_object = M2MInterfaceFactory::create_object(object_name.c_str());
    if (nullptr == m2m_object) {
        TR_ERR("Create m2m_object: %s failed", object_name.c_str());
        return Error::CCRBCreateM2MObjFailed;
    }
    TR_DEBUG("Created m2m_object: %s", m2m_object->name());
    m2m_object_list.push_back(m2m_object);

    for (auto itr = object_definition.begin(); itr != object_definition.end(); itr++) {
        Json::Value object_instance_definition = *itr;
        Json::Value object_instance_id = itr.key();
        if (parse_object_instance(std::stoi(object_instance_id.asString()),
                                  object_instance_definition,
                                  m2m_object) != Error::None)
        {
            TR_ERR("parse_object_instance failed.");
            return Error::CCRBInvalidJson;
        }
    }

    return Error::None;
}

std::pair<MblError, M2MObjectList>
ResourceDefinitionParser::build_object_list(const std::string& application_resource_definition)
{
    TR_DEBUG_ENTER;

    std::pair<MblError, M2MObjectList> ret_pair(MblError::None, M2MObjectList());

    // We must catch exception to avoid crashes when application send invalid /
    // corrupted JSON
    try
    {
        Json::CharReaderBuilder builder;
        Json::Value json_settings;

        // Set strict mode
        builder.strictMode(&json_settings); // Init json_settings with strict mode values

        // avoid failing on extra white space
        json_settings["failIfExtra"] = false;

        builder.settings_ = json_settings;
        Json::CharReader* reader = builder.newCharReader();
        Json::Value root;
        std::string errors;

        // Parse
        const char* end_string = &*application_resource_definition.cend();
        bool parsing_successful =
            reader->parse(application_resource_definition.c_str(), end_string, &root, &errors);
        delete reader;
        if (!parsing_successful) {
            TR_ERR("parsing Json string failed with errors: %s.", errors.c_str());
            ret_pair.first = Error::CCRBInvalidJson;
            return ret_pair;
        }
        if (root.empty()) {
            TR_ERR("Invalid JSON. Root is empty.");
            ret_pair.first = Error::CCRBInvalidJson;
            return ret_pair;
        }
        // Parse all objects:
        for (auto itr = root.begin(); itr != root.end(); itr++) {
            Json::Value object_definition = *itr;
            Json::Value object_name = itr.key();
            ret_pair.first =
                parse_object(object_name.asString(), object_definition, ret_pair.second);
            if (ret_pair.first != Error::None) {
                TR_ERR("parse_object failed with error %s", MblError_to_str(ret_pair.first));
                break;
            }
        }
    } catch (Json::Exception& e)
    {
        TR_ERR("BuildResourceList failed with Json::Exception exception msg: %s.", e.what());
        ret_pair.first = Error::CCRBInvalidJson;
    } catch (std::runtime_error& e)
    {
        TR_ERR("BuildResourceList failed with runtime_error exception msg: %s.", e.what());
        ret_pair.first = Error::CCRBInvalidJson;
    } catch (std::exception& e)
    {
        TR_ERR("BuildResourceList failed with std::exception exception msg: %s.", e.what());
        ret_pair.first = Error::CCRBInvalidJson;
    }

    // Free allocated memory for object instances in case error occured
    if (ret_pair.first != Error::None) {
        // Clear m2m_object list
        M2MObject* m2m_object = nullptr;
        for (auto& itr : ret_pair.second) {
            m2m_object = itr;
            TR_DEBUG("Deleting m2m_object: %s", m2m_object->name());
            delete m2m_object; // This will delete all created object instances and
                               // all resources that belongs to it
        }
        ret_pair.second.clear();
    }
    return ret_pair;
}

} // namespace mbl
