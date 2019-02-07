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

namespace mbl {

class RBM2MResource {

friend class RBM2MObjectInstance;

public:

    /**
     * @brief Set M2MResource
     * 
     * @param m2m_resource - M2MResource
     */
    void set_m2m_resource(M2MResource *m2m_resource);

    /**
     * @brief Get M2MResource
     * 
     * @return M2MResource* 
     */
    M2MResource* get_m2m_resource();

    /**
     * @brief Get RBM2MResource name
     * 
     * @return const std::string& 
     */
    const std::string& get_resource_name() const;

    /**
     * @brief Get RBM2MResource mode (e.g. M2MBase::Static)
     * 
     * @return M2MBase::Mode 
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
     * @return M2MBase::Operation 
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
     * @return const std::string& 
     */
    const std::string& get_resource_type() const;

    /**
     * @brief Get RBM2MResource type (e.g. M2MResourceInstance::INTEGER)
     * 
     * @return M2MResourceBase::ResourceType 
     */
    M2MResourceBase::ResourceType get_type() const;

    /**
     * @brief Get RBM2MResource value As String
     * 
     * @return const std::string& 
     */
    const std::string& get_value_as_string() const;

    /**
     * @brief Get RBM2MResource value As Integer
     * 
     * @return int 
     */
    int get_value_as_integer() const;

private:
    // Constructor and destructor are private
    // which means that these objects can be created or
    // deleted only through a function provided by RBM2MObjectInstance.
    RBM2MResource(
        std::string resource_name,
        M2MBase::Mode mode,
        bool multiple_instances,
        M2MBase::Operation operation,
        bool observable,
        const std::string &resource_type,
        M2MResourceBase::ResourceType type,
        const std::string &value);
    ~RBM2MResource();

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

typedef std::map<std::string, RBM2MResource*> RBM2MResourceMap;

////////////////////////////////////////////////////////////////////////////////

class RBM2MObjectInstance {

friend class RBM2MObject;

public:

    /**
     * @brief Set M2MObjectInstance
     * 
     * @param m2m_object_instance - M2MObjectInstance
     */
    void set_m2m__object_instance(M2MObjectInstance* m2m_object_instance);

    /**
     * @brief Get M2MObjectInstance
     * 
     * @return M2MObjectInstance* 
     */
    M2MObjectInstance* get_m2m_object_instance();

    /**
     * @brief Get Object Instance Id
     * 
     * @return uint16_t 
     */
    uint16_t get_object_instance_id() const;

    /**
     * @brief Get RBM2MResource Map
     * 
     * @return const RBM2MResourceMap& 
     */
    const RBM2MResourceMap& get_resource_map() const;

    /**
     * @brief Create a RBM2MResource
     * 
     * @param resource_name - Resource name
     * @param mode - Mode
     * @param multiple_instances - true if support multiple instances
     * @param operation - Allowed resource operation
     * @param observable - true if observable (relevant only for dynamic resources)
     * @param resource_type - Resource type (e.g. "button")
     * @param type - Type (e.g. M2MResourceInstance::INTEGER)
     * @param value - Value (Integer values are also kept as string)
     * @return RBM2MResource* 
     */
    RBM2MResource* create_resource(
        const std::string &resource_name,
        M2MBase::Mode mode,
        bool multiple_instances,
        M2MBase::Operation operation,
        bool observable,
        const std::string &resource_type,
        M2MResourceBase::ResourceType type,
        const std::string &value);

private:
    // Constructor and destructor are private
    // which means that these objects can be created or
    // deleted only through a function provided by RBM2MObject.
    RBM2MObjectInstance(uint16_t object_instance_id);
    ~RBM2MObjectInstance();

    uint16_t object_instance_id_;
    M2MObjectInstance* m2m_object_instance_; // Associated M2MObjectInstance
    RBM2MResourceMap rbm2m_resource_map_;
};

typedef std::map<uint16_t, RBM2MObjectInstance*> RBM2MObjectInstanceMap;

////////////////////////////////////////////////////////////////////////////////

class RBM2MObject {

friend class RBM2MObjectList;    

public:

    /**
     * @brief Set M2MObject
     * 
     * @param m2m_object - M2MObject
     */
    void set_m2m_object(M2MObject *m2m_object);

    /**
     * @brief Get M2MObject
     * 
     * @return M2MObject* 
     */
    M2MObject* get_m2m_object();

    /**
     * @brief Get RBM2MObject Name
     * 
     * @return const std::string& 
     */
    const std::string& get_object_name() const;

    /**
     * @brief Get RBM2MObjectInstance Map
     * 
     * @return const RBM2MObjectInstanceMap& 
     */
    const RBM2MObjectInstanceMap& get_object_instance_map() const;

    /**
     * @brief Create RBM2MObjectInstance
     * 
     * @param object_instance_id - RBM2MObjectInstance id
     * @return RBM2MObjectInstance* 
     */
    RBM2MObjectInstance* create_object_instance(uint16_t object_instance_id);
   
private:
    // Constructor and destructor are private
    // which means that these objects can be created or
    // deleted only through a function provided by the RBM2MObjectList.
    RBM2MObject(std::string object_name);
    ~RBM2MObject();

    std::string object_name_;
    M2MObject *m2m_object_; // Assosiated M2MObject
    RBM2MObjectInstanceMap rbm2m_object_instance_map_;
};

typedef std::map<std::string, RBM2MObject*> RBM2MObjectMap;

////////////////////////////////////////////////////////////////////////////////

class RBM2MObjectList {
    
public:
    RBM2MObjectList();
    ~RBM2MObjectList();

    /**
     * @brief Get RBM2MObject Map
     * 
     * @return const RBM2MObjectMap& 
     */
    const RBM2MObjectMap& get_object_map() const;

    /**
     * @brief Create RBM2MObject
     * 
     * @param object_name - RBM2MObject name
     * @return RBM2MObject* 
     */
    RBM2MObject* create_object(const std::string &object_name);

    /**
     * @brief Get RBM2MObject by its name
     * 
     * @param object_name - RBM2MObject name
     * @return RBM2MObject* 
     */
    RBM2MObject* get_object(const std::string &object_name);

private:
    RBM2MObjectMap rbm2m_object_map_;
};

} // namespace mbl

#endif // M2MResourceObjects_h_
