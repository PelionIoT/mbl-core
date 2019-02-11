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


#ifndef M2MResourceObjects_h_
#define M2MResourceObjects_h_

#include "MblError.h"
#include "mbed-client/m2minterface.h"
#include <map>
#include <string>
#include <memory>

namespace mbl {

/**
 * @brief RBM2MResource represent application resource.
 * It holds all relevant information such as name, mode (e.g. static / dynamic),
 * resource type (integer/string), resource value and more.
 * It also contains a pointer to Mbed cloud client m2m corresponding resource (but it is NOT its owner!)
  */
class RBM2MResource {

public:

    /**
     * @brief Construct a new RBM2MResource object
     * 
     * @param resource_name - Resource name
     * @param mode - Resource mode dynamic / static
     * @param multiple_instances - Signals if multiple instances allowed
     * @param operation - Combination of allowed operation, e.g. get, put, post, delete
     * @param observable - Signals if the resource is observable
     * @param resource_type - String that describes resource type (e.g. "reset_button")
     * @param type - Resource type string / integer
     * @param value - Resource value
     */
    RBM2MResource(
        std::string resource_name,
        M2MBase::Mode mode,
        bool multiple_instances,
        M2MBase::Operation operation,
        bool observable,
        std::string resource_type,
        M2MResourceBase::ResourceType type,
        std::string value);

    /**
     * @brief Destroy the RBM2MResource object
     * 
     */
    ~RBM2MResource();

    /**
     * @brief Set correcponding Mben client M2MResource
     * m2m_resource ownership is not changed by this function call!
     * 
     * @param m2m_resource - Mben client M2MResource
     */
    void set_m2m_resource(M2MResource *m2m_resource);

    /**
     * @brief Get corresponding Mbed cloud client M2MResource
     * 
     * @return Pointer to Mbed cloud client M2MResource
     */
    M2MResource* get_m2m_resource();

    /**
     * @brief Get RBM2MResource name
     * 
     * @return Resource name
     */
    const std::string& get_resource_name() const;

    /**
     * @brief Get RBM2MResource mode (e.g. M2MBase::Static)
     * 
     * @return Resource M2MBase::Mode (dynamic / static)
     */
    M2MBase::Mode get_mode() const;

    /**
     * @brief Get Supports Multiple Instances property
     * 
     * @return true - if multiple instances is supported
     * @return false - if multiple instances is not supported 
     */
    bool get_supports_multiple_instances() const;

    /**
     * @brief Get RBM2MResource allowed operations
     * 
     * @return Mbed cloud client M2MBase::Operation (e.g. put, get, delete, post and their combination)
     */
    M2MBase::Operation get_operations() const;

    /**
     * @brief Get Observable property (relevant only for dynamic resources)
     * 
     * @return true - if resource is observable
     * @return false - if resource is not observable
     */
    bool get_observable() const;

    /**
     * @brief Get RBM2MResource resource type (e.g. "button")
     * 
     * @return Resource type (e.g. "reset_button")
     */
    const std::string& get_resource_type() const;

    /**
     * @brief Get RBM2MResource type (e.g. M2MResourceInstance::INTEGER)
     * 
     * @return Mbed cloud client M2MResourceBase::ResourceType (e.g. integer / string)
     */
    M2MResourceBase::ResourceType get_type() const;

    /**
     * @brief Get RBM2MResource value As String
     * 
     * @return Resource value as string
     */
    const std::string& get_value_as_string() const;

    /**
     * @brief Get RBM2MResource value As Integer
     * 
     * @return Resource value as integer
     */
    int get_value_as_integer() const;

private:
    std::string resource_name_;
    M2MBase::Mode mode_;
    bool multiple_instances_;
    M2MBase::Operation operation_;
    bool observable_;
    std::string resource_type_;
    M2MResourceBase::ResourceType type_;
    std::string value_;
    M2MResource *m2m_resource_; // Assosiate M2M object
};

typedef std::unique_ptr<RBM2MResource> SPRBM2MResource;
typedef std::map<std::string, SPRBM2MResource> RBM2MResourceMap;

////////////////////////////////////////////////////////////////////////////////

/**
 * @brief RBM2MObjectInstance represent application object instance.
 * It is identified by object instance ID (uint16_t) and contains a map that holds
 * all resources belongs to it.
 * It also contains a pointer to Mbed cloud client m2m corresponding object instance (but it is NOT its owner!)
  */
class RBM2MObjectInstance {

public:

    /**
     * @brief Construct a new RBM2MObjectInstance object
     * 
     * @param object_instance_id - Object instance id
     */
    RBM2MObjectInstance(uint16_t object_instance_id);

    /**
     * @brief Destroy the RBM2MObjectInstance object
     * 
     */
    ~RBM2MObjectInstance();
    /**
     * @brief Set corresponding Mben client M2MObjectInstance
     * m2m_object_instance ownership is not changed by this function call!
     * 
     * @param m2m_object_instance - Mben client M2MObjectInstance
     */
    void set_m2m_object_instance(M2MObjectInstance* m2m_object_instance);

    /**
     * @brief Get the corresponding Mbed cloud client M2MObjectInstance
     * 
     * @return The corresponding Mbed cloud client M2MObjectInstance
     */
    M2MObjectInstance* get_m2m_object_instance();

    /**
     * @brief Get Object Instance Id
     * 
     * @return Object Instance Id
     */
    uint16_t get_object_instance_id() const;

    /**
     * @brief Get RBM2MResource Map that holds all child resources
     * 
     * @return RBM2MResource Map that holds all child resources
     */
    const RBM2MResourceMap& get_resource_map() const;

    /**
     * @brief Create a SPRBM2MResource
     * 
     * @param resource_name - Resource name
     * @param mode - Mode
     * @param multiple_instances - true if support multiple instances
     * @param operation - Allowed resource operation
     * @param observable - true if observable (relevant only for dynamic resources)
     * @param resource_type - Resource type (e.g. "button")
     * @param type - Type (e.g. M2MResourceInstance::INTEGER)
     * @param value - Value (Integer values are also kept as string)
     * @return Smart pointer to RBM2MResource
     */
    SPRBM2MResource* create_resource(
        const std::string &resource_name,
        M2MBase::Mode mode,
        bool multiple_instances,
        M2MBase::Operation operation,
        bool observable,
        const std::string &resource_type,
        M2MResourceBase::ResourceType type,
        const std::string &value);

private:
    uint16_t object_instance_id_;
    M2MObjectInstance* m2m_object_instance_; // Corresponding Mbed cloud client M2MObjectInstance
    RBM2MResourceMap rbm2m_resource_map_;
};

typedef std::unique_ptr<RBM2MObjectInstance> SPRBM2MObjectInstance;
typedef std::map<uint16_t, SPRBM2MObjectInstance> RBM2MObjectInstanceMap;

////////////////////////////////////////////////////////////////////////////////

class RBM2MObject {

public:

    /**
     * @brief Construct a new RBM2MObject object
     * 
     * @param Object name 
     */
    RBM2MObject(std::string object_name);

    /**
     * @brief Destroy the RBM2MObject object
     * 
     */
    ~RBM2MObject();

    /**
     * @brief Set corresponding Mben client M2MObject
     * m2m_object ownership is not changed by this function call!
     * 
     * @param m2m_object - Corresponding Mben client M2MObject
     */
    void set_m2m_object(M2MObject *m2m_object);

    /**
     * @brief Get the corresponding Mbed cloud client M2MObject
     * 
     * @return The corresponding Mbed cloud client M2MObject
     */
    M2MObject* get_m2m_object();

    /**
     * @brief Get RBM2MObject Name
     * 
     * @return Object name
     */
    const std::string& get_object_name() const;

    /**
     * @brief Get RBM2MObjectInstance Map
     * 
     * @return const RBM2MObjectInstanceMap& 
     */
    const RBM2MObjectInstanceMap& get_object_instance_map() const;

    /**
     * @brief Create SPRBM2MObjectInstance
     * 
     * @param object_instance_id - RBM2MObjectInstance id
     * @return Smart pointer to RBM2MObjectInstance
     */
    SPRBM2MObjectInstance* create_object_instance(uint16_t object_instance_id);
   
private:
    std::string object_name_;
    M2MObject *m2m_object_; // Assosiated M2MObject
    RBM2MObjectInstanceMap rbm2m_object_instance_map_;
};

typedef std::unique_ptr<RBM2MObject> SPRBM2MObject;
typedef std::map<std::string, SPRBM2MObject> RBM2MObjectMap;

////////////////////////////////////////////////////////////////////////////////

class RBM2MObjectList {
    
public:
    RBM2MObjectList();
    ~RBM2MObjectList();

    /**
     * @brief Get Get RBM2MObject Map that holds all child object instances
     * 
     * @return RBM2MObject Map
     */
    const RBM2MObjectMap& get_object_map() const;

    /**
     * @brief Clear all objects in the map
     * 
     */
    void clear_object_map();

    /**
     * @brief Create SPRBM2MObject
     * 
     * @param object_name - RBM2MObject name
     * @return Smart pointer to RBM2MObject
     */
    SPRBM2MObject* create_object(const std::string &object_name);

    /**
     * @brief Get SPRBM2MObject by its name
     * 
     * @param object_name - RBM2MObject name
     * @return Smart pointer to RBM2MObject
     */
    SPRBM2MObject* get_object(const std::string &object_name);

private:
    RBM2MObjectMap rbm2m_object_map_;
};

} // namespace mbl

#endif // M2MResourceObjects_h_
