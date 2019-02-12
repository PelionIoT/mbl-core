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

#include "M2MResourceObjects.h"
#include "mbed-trace/mbed_trace.h"

#define TRACE_GROUP "ccrb-m2mobjects"

namespace mbl {

////////////////////////////////////////////////////////////////////////////////
// RBM2MResource
////////////////////////////////////////////////////////////////////////////////
RBM2MResource::RBM2MResource(
    std::string resource_name,
    M2MBase::Mode mode,
    bool multiple_instances,
    M2MBase::Operation operation,
    bool observable,
    std::string resource_type,
    M2MResourceBase::ResourceType type,
    std::string value)
: resource_name_(std::move(resource_name)), 
mode_(mode),
multiple_instances_(multiple_instances),
operation_(operation),
observable_(observable),
resource_type_(std::move(resource_type)),
type_(type),
value_(std::move(value)),
m2m_resource_(nullptr)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    tr_debug("resource_name: %s", resource_name_.c_str());
    tr_debug("mode: %d", static_cast<int>(mode_));
    tr_debug("multiple_instances: %s", multiple_instances_ ? "true" : "false");
    tr_debug("operation: %d", static_cast<int>(operation_));
    tr_debug("observable: %s", observable_ ? "true" : "false");
    tr_debug("resource_type: %s", resource_type.c_str());
    tr_debug("type: %d", static_cast<int>(type_));
    tr_debug("value: %s", value_.c_str());
}

RBM2MResource::~RBM2MResource()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

void RBM2MResource::set_m2m_resource(M2MResource *m2m_resource)
{
    m2m_resource_ = m2m_resource; // It is allowed to set nullptr M2MResource
}

M2MResource* RBM2MResource::get_m2m_resource()
{
    return m2m_resource_;
}

const std::string& RBM2MResource::get_resource_name() const 
{
    return resource_name_;
}

M2MBase::Mode RBM2MResource::get_mode() const 
{
    return mode_;
}

bool RBM2MResource::get_supports_multiple_instances() const 
{
    return multiple_instances_;
}

M2MBase::Operation RBM2MResource::get_operations() const 
{
    return operation_;
}

bool RBM2MResource::get_observable() const 
{
    return observable_;
}

const std::string& RBM2MResource::get_resource_type() const 
{
    return resource_type_;
}

M2MResourceBase::ResourceType RBM2MResource::get_type() const 
{
    return type_;
}

const std::string& RBM2MResource::get_value_as_string() const
{
    return value_;
}

MblError RBM2MResource::get_value_as_integer(int &value) const
{
    if(M2MResourceBase::INTEGER != type_) {
        tr_error("%s - Value type is not integer", __PRETTY_FUNCTION__);
        return Error::CCRBValueTypeError;
    }
    value = std::stoi(value_);
    return Error::None;
}

////////////////////////////////////////////////////////////////////////////////
// RBM2MObjectInstance
////////////////////////////////////////////////////////////////////////////////
RBM2MObjectInstance::RBM2MObjectInstance(uint16_t object_instance_id)
    : object_instance_id_(object_instance_id), m2m_object_instance_(nullptr)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

RBM2MObjectInstance::~RBM2MObjectInstance()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    rbm2m_resource_map_.clear();
}

void RBM2MObjectInstance::set_m2m_object_instance(M2MObjectInstance* m2m_object_instance)
{
    m2m_object_instance_ = m2m_object_instance;
}

M2MObjectInstance* RBM2MObjectInstance::get_m2m_object_instance()
{
    return m2m_object_instance_;
}


uint16_t RBM2MObjectInstance::get_object_instance_id() const
{
    return object_instance_id_;
}

const RBM2MResourceMap& RBM2MObjectInstance::get_resource_map() const
{
    return rbm2m_resource_map_;
}

SPRBM2MResource RBM2MObjectInstance::create_resource(
    const std::string &resource_name,
    M2MBase::Mode mode,
    bool multiple_instances,
    M2MBase::Operation operation,
    bool observable,
    const std::string &resource_type,
    M2MResourceBase::ResourceType type,
    const std::string &value)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    if(resource_name.empty()) {
        tr_error("%s - resource name is empty", __PRETTY_FUNCTION__);
        return nullptr;
    }

    //Verify resource does not exist
    auto itr = rbm2m_resource_map_.find(resource_name);
    if(itr != rbm2m_resource_map_.end()) {
        tr_error("%s - resource %s already exist",__PRETTY_FUNCTION__, resource_name.c_str());
        return nullptr;
    }

    tr_debug("Created rbm2m resource: %s", resource_name.c_str());
    rbm2m_resource_map_[resource_name] = std::make_shared<RBM2MResource>(
        resource_name,
        mode,
        multiple_instances,
        operation,
        observable,
        resource_type,
        type,
        value);
    
    return rbm2m_resource_map_[resource_name];
}

////////////////////////////////////////////////////////////////////////////////
// RBM2MObject
////////////////////////////////////////////////////////////////////////////////
RBM2MObject::RBM2MObject(std::string object_name)
    : object_name_(std::move(object_name)), m2m_object_(nullptr)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

RBM2MObject::~RBM2MObject()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    rbm2m_object_instance_map_.clear();
}

void RBM2MObject::set_m2m_object(M2MObject *m2m_object)
{
    m2m_object_ = m2m_object;
}

M2MObject* RBM2MObject::get_m2m_object()
{
    return m2m_object_;
}

const std::string& RBM2MObject::get_object_name() const
{
    return object_name_;
}

const RBM2MObjectInstanceMap& RBM2MObject::get_object_instance_map() const
{
    return rbm2m_object_instance_map_;
}

SPRBM2MObjectInstance RBM2MObject::create_object_instance(uint16_t object_instance_id)
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    //Verify object_instance does not exist
    auto itr = rbm2m_object_instance_map_.find(object_instance_id);
    if(itr != rbm2m_object_instance_map_.end()) {
        tr_error("%s - object instance %" PRId16 " already exist", __PRETTY_FUNCTION__, object_instance_id);
        return nullptr;
    }

    rbm2m_object_instance_map_[object_instance_id] = std::make_shared<RBM2MObjectInstance>(object_instance_id);    
    return rbm2m_object_instance_map_[object_instance_id];
}

////////////////////////////////////////////////////////////////////////////////
// RBM2MObjectList
////////////////////////////////////////////////////////////////////////////////

RBM2MObjectList::RBM2MObjectList()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

RBM2MObjectList::~RBM2MObjectList()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    clear_object_map();
}

void RBM2MObjectList::clear_object_map()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    rbm2m_object_map_.clear();
}


const RBM2MObjectMap& RBM2MObjectList::get_object_map() const
{
    return rbm2m_object_map_;
}

SPRBM2MObject RBM2MObjectList::create_object(const std::string &object_name)
{
    tr_debug("%s", __PRETTY_FUNCTION__);    
    if(object_name.empty()) {
        tr_error("%s - object name is empty", __PRETTY_FUNCTION__);
        return nullptr;
    }

    //Verify object does not exist
    auto itr = rbm2m_object_map_.find(object_name);
    if(itr != rbm2m_object_map_.end()) {
        tr_error("%s - object %s already exist", __PRETTY_FUNCTION__, object_name.c_str());
        return nullptr;
    }

    rbm2m_object_map_[object_name] = std::make_shared<RBM2MObject>(object_name);    
    return rbm2m_object_map_[object_name];
}

SPRBM2MObject RBM2MObjectList::get_object(const std::string &object_name)
{
    // Check if object exist
    auto itr = rbm2m_object_map_.find(object_name);
    if(itr != rbm2m_object_map_.end()) {
        return itr->second;
    }
    tr_info("%s: Object %s does not exist", __PRETTY_FUNCTION__, object_name.c_str());
    return nullptr;
}

} // namespace mbl
