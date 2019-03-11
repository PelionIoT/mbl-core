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

#include "RegistrationRecord.h"
#include "CloudConnectTrace.h"
#include "MbedCloudClient.h"
#include "ResourceDefinitionParser.h"

#include <algorithm>

#define PATH_SEPERATOR '/'

// In case of object/object_instance,resource - depth is 3
// In case of object/object_instance,resource/resource_instance - depth is 4
static const size_t RESOURCE_DEPTH = 3;
static const size_t RESOURCE_INSTANCE_DEPTH = 4;
static uint8_t OBJECT_INDEX = 0;
static uint8_t OBJECT_INSTANCE_INDEX = 1;
static uint8_t RESOURCE_INDEX = 2;
static uint8_t RESOURCE_INSTANCE_INDEX = 3;

#define TRACE_GROUP "ccrb-registration-record"

namespace mbl
{

RegistrationRecord::RegistrationRecord(const IpcConnection& source)
    : source_(source), registered_(false)
{
    TR_DEBUG_ENTER;
}

RegistrationRecord::~RegistrationRecord()
{
    TR_DEBUG_ENTER;
}

MblError RegistrationRecord::init(const std::string& application_resource_definition)
{
    TR_DEBUG_ENTER;

    // Parse JSON
    std::pair<MblError, M2MObjectList> ret_pair =
        ResourceDefinitionParser::build_object_list(application_resource_definition);

    if (Error::None != ret_pair.first) {
        TR_ERR("build_object_list failed with error: %s", MblError_to_str(ret_pair.first));
        return ret_pair.first;
    }

    m2m_object_list_ = ret_pair.second;
    return Error::None;
}

MblError RegistrationRecord::get_resource_identifiers(const std::string& path,
                                                      std::string& out_object_name,
                                                      uint16_t& out_object_instance_id,
                                                      std::string& out_resource_name,
                                                      std::string& out_resource_instance_name)
{
    TR_DEBUG_ENTER;

    std::size_t current_pos = path.find_first_of(PATH_SEPERATOR);

    // Make sure path starts with "/"
    if (0 != current_pos) {
        TR_ERR("Invalid path: %s, should start with '/'", path.c_str());
        return Error::CCRBInvalidResourcePath;
    }

    std::size_t last_pos = path.find_last_of(PATH_SEPERATOR);
    if (last_pos == path.length() - 1) {
        TR_ERR("Invalid path: %s, should end with '/'", path.c_str());
        return Error::CCRBInvalidResourcePath;
    }

    // Split all identifiers by path seperator and push into resource_identifiers vector
    std::string substr;
    std::size_t previous_pos = 0;
    std::vector<std::string> resource_identifiers;
    while (current_pos != std::string::npos) {
        substr = path.substr(previous_pos, current_pos - previous_pos);
        if (!substr.empty()) {
            resource_identifiers.push_back(substr);
        }
        previous_pos = current_pos + 1;
        current_pos = path.find_first_of(PATH_SEPERATOR, previous_pos);
        if (current_pos == previous_pos) {
            TR_ERR("Invalid path: %s, Two subsequent '/' are not allowed", path.c_str());
            return Error::CCRBInvalidResourcePath;
        }
    }
    resource_identifiers.push_back(path.substr(previous_pos, current_pos - previous_pos));

    // In case of object/object_instance,resource - depth is 3
    // In case of object/object_instance,resource/resource_instance - depth is 4
    if ((resource_identifiers.size() < RESOURCE_DEPTH) ||
        (resource_identifiers.size() > RESOURCE_INSTANCE_DEPTH))
    {
        TR_ERR("Invalid path: %s, depth = %d", path.c_str(), (int) resource_identifiers.size());
        return Error::CCRBInvalidResourcePath;
    }

    // Object:
    out_object_name = resource_identifiers[OBJECT_INDEX];
    TR_DEBUG("object_name: %s", out_object_name.c_str());

    // Object instance:
    bool is_digit =
        std::all_of(resource_identifiers[1].begin(), resource_identifiers[1].end(), ::isdigit);
    if (!is_digit) {
        TR_ERR("Invalid path: %s, object instance id: %s is not a number",
               path.c_str(),
               resource_identifiers[OBJECT_INSTANCE_INDEX].c_str());
        return Error::CCRBInvalidResourcePath;
    }
    int object_instance_id = 0;
    try
    {
        object_instance_id = std::stoi(resource_identifiers[OBJECT_INSTANCE_INDEX]);
    } catch (std::out_of_range& e)
    {
        TR_ERR("Invalid path: %s, out_of_range object instance id value: %s",
               path.c_str(),
               resource_identifiers[OBJECT_INSTANCE_INDEX].c_str());
        return Error::CCRBInvalidResourcePath;
    } catch (std::invalid_argument& e)
    {
        TR_ERR("Invalid path: %s, invalid argument instance id value: %s",
               path.c_str(),
               resource_identifiers[OBJECT_INSTANCE_INDEX].c_str());
        return Error::CCRBInvalidResourcePath;
    }
    if (object_instance_id < 0 || object_instance_id > UINT16_MAX) {
        TR_ERR("Invalid path %s, object_instance_id allowed value should be between 0 and %d",
               path.c_str(),
               UINT16_MAX);
        return Error::CCRBInvalidResourcePath;
    }
    out_object_instance_id = static_cast<uint16_t>(object_instance_id);
    TR_DEBUG("object_instance_id: %d", object_instance_id);

    // Resource:
    out_resource_name = resource_identifiers[RESOURCE_INDEX];
    TR_DEBUG("resource_name: %s", out_resource_name.c_str());

    // Resource instance:
    if (resource_identifiers.size() == RESOURCE_INSTANCE_DEPTH) {
        out_resource_instance_name = resource_identifiers[RESOURCE_INSTANCE_INDEX];
        TR_DEBUG("resource_instance_name: %s", out_resource_instance_name.c_str());
    }
    return Error::None;
}

std::pair<MblError, M2MResource*> RegistrationRecord::get_m2m_resource(const std::string& path)
{
    TR_DEBUG("path: %s", path.c_str());

    std::pair<MblError, M2MResource*> ret_pair(MblError::None, nullptr);

    std::string object_name, resource_name, resource_instance_name;
    uint16_t object_instance_id = 0;
    const MblError status = get_resource_identifiers(
        path, object_name, object_instance_id, resource_name, resource_instance_name);
    if (Error::None != status) {
        TR_ERR("get_resource_identifiers failed with error: %s", MblError_to_str(status));
        ret_pair.first = status;
        return ret_pair;
    }

    // TODO: Remove next check when we will support resource instances
    if (!resource_instance_name.empty()) {
        TR_ERR("Resource instances are not supported (%s)", path.c_str());
        ret_pair.first = MblError::CCRBInvalidResourcePath;
        return ret_pair;
    }

    M2MResource* m2m_resource = nullptr;
    for (auto& obj_itr : m2m_object_list_) {
        // Object names and object instances are unique as we use strict mode app definition parsing

        // Object names are unique
        M2MObject* m2m_object = obj_itr;
        if (object_name != m2m_object->name()) {
            continue;
        }

        // Object instance is unique under a specific Object
        M2MObjectInstance* m2m_object_instance = m2m_object->object_instance(object_instance_id);
        if (nullptr == m2m_object_instance) {
            continue;
        }

        // Found! Resource is unique under specific object
        m2m_resource = m2m_object_instance->resource(resource_name.c_str());
        break;
    }
    ret_pair.second = m2m_resource;

    if (nullptr == ret_pair.second) {
        TR_ERR("Resource %s not found", path.c_str());
        ret_pair.first = Error::CCRBResourceNotFound;
    }

    return ret_pair;
}

} // namespace mbl
