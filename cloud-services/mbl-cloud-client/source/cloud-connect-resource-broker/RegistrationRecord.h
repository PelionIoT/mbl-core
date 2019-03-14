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


#ifndef RegistrationRecord_h_
#define RegistrationRecord_h_

#include "MblError.h"
#include "CloudConnectTypes.h"
#include <map>
#include <string>
#include <memory>
#include <functional>

class RegistrationRecordTester;

namespace mbl {


/**
 * @brief This class represent an Application endpoint, holds M2M resources, access token and more.
 * This class register for Mbed cloud client callbacks, and when called - pass the relevant information
 * to ResourceBroker.
 */
class RegistrationRecord {

friend ::RegistrationRecordTester;

public:

    RegistrationRecord(IpcConnection registration_source);
    ~RegistrationRecord();

    /**
     * @brief Initialize Application resource M2M lists using JSON string
     * 
     * @param application_resource_definition - Application resource definition JSON string
     * @return MblError -
     *      Error::None - If function succeeded
     *      Error::CCRBInvalidJson - I case of invalid JSON (e.g. Invalid JSON structure or invalid M2M content such as missing mandatory entries)
     *      Error::CCRBCreateM2MObjFailed - If create of M2M object/object instance/resource failed
     *      Error::CCRBGenerateUniqueIdFailed - In case unique access token creation failed
     */
    MblError init(const std::string& application_resource_definition);

    // TODO: add description
    MblError update_ipc_connection(IpcConnection &source, bool closed);

    /**
     * @brief Get resource by its path
     * 
     * @param path - resource path (e.g. "/8888/11/1")
     * @return std::pair<MblError, M2MResource*> -
     *          first element in the pair is error code:
     *              Error::CCRBInvalidResourcePath - In case of invalid path
     *              Error::CCRBResourceNotFound - In case resource not found
     *              Error::None - In case of success
     *          second element is a pointer to resource
     * Note: resource pointer is valid only if error code is Error::None
     */
    std::pair<MblError, M2MResource*> get_m2m_resource(const std::string& path);

    /**
     * @brief Set registered status
     * 
     * @param registered - true if registered, false if not registered.
     */
    inline void set_registered(bool registered) {registered_ = registered;}

    /**
     * @brief Return registration ipc connection
     * 
     * @return ipc request handle
     */
    const IpcConnection& get_registration_source() const { return registration_source_; }

    /**
     * @brief Return registered status
     * 
     * @return true if registered
     * @return false if not registered
     */
    inline bool is_registered() const {return registered_;}

    /**
     * @brief Return m2m object list object
     * 
     * @return m2m object list object
     */
    inline M2MObjectList& get_m2m_object_list() {return m2m_object_list_;}

private:

    /**
     * @brief Get resource identifiers by path
     * 
     * @param path - resource path (e.g. "/8888/112/11/1")
     * @param out_object_name - Object name (e.g. "8888")
     * @param out_object_instance_id  - Object instance id (e.g. 112)
     * @param out_resource_name - Resource name (e.g. "11")
     * @param out_resource_instance_name - Resource instance name (e.g. "1")
     * @return MblError -
     *              Error::CCRBInvalidResourcePath - In case of invalid path
     *              Error::None - In case of success
     */
    static MblError get_resource_identifiers(const std::string& path,
                                      std::string& out_object_name,
                                      uint16_t& out_object_instance_id,
                                      std::string& out_resource_name,
                                      std::string& out_resource_instance_name);

    IpcConnection registration_source_;
    std::vector<IpcConnection> ipc_connections_;
    bool registered_;

    M2MObjectList m2m_object_list_; // Cloud client M2M object list used for registration
};

} // namespace mbl

#endif // RegistrationRecord_h_
