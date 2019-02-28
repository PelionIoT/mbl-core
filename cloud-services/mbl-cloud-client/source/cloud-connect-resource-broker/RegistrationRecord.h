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

#include <map>
#include <string>
#include <memory>
#include <functional>

namespace mbl {

typedef std::function<void(const uintptr_t, const std::string&)> app_register_update_finished_func;
typedef std::function<void(const uintptr_t, const std::string&, const MblError)> app_error_func;

/**
 * @brief This class represent an Application endpoint, holds M2M resources, access token and more.
 * This class register for Mbed cloud client callbacks, and when called - pass the relevant information
 * to ResourceBroker.
 */
class RegistrationRecord {

public:

    RegistrationRecord(const uintptr_t ipc_request_handle);
    ~RegistrationRecord();

    /**
     * @brief Initialize Application resource M2M lists using JSON string
     * 
     * @param application_resource_definition - Application resource definition JSON string
     * @return MblError -
     *      Error::None - If function succeeded
     *      Error::CCRBInvalidJson - In case of invalid JSON (e.g. Invalid JSON structure or invalid M2M content such as missing mandatory entries)
     *      Error::CCRBCreateM2MObjFailed - If create of M2M object/object instance/resource failed
     *      Error::CCRBGenerateUniqueIdFailed - In case unique access token creation failed
     */
    MblError init(const std::string& application_resource_definition);

    /**
     * @brief Set registered status
     * 
     * @param registered - true if registered, false if not registered.
     */
    inline void set_registered(bool registered) {registered_ = registered;}

    /**
     * @brief Return ipc request handle
     * 
     * @return ipc request handle
     */
    inline uintptr_t get_ipc_request_handle() const {return ipc_request_handle_;};

    /**
     * @brief Return registered status
     * 
     * @return true if registerd
     * @return false if not registerd
     */
    inline bool is_registered() const {return registered_;}

    /**
     * @brief Return m2m object list object
     * 
     * @return m2m object list object
     */
    inline M2MObjectList& get_m2m_object_list() {return m2m_object_list_;}

private:

    uintptr_t ipc_request_handle_;
    bool registered_;

    M2MObjectList m2m_object_list_; // Cloud client M2M object list used for registration
};

} // namespace mbl

#endif // RegistrationRecord_h_
