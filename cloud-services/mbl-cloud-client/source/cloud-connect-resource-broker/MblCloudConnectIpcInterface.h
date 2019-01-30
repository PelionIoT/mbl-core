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


#ifndef MblCloudConnectIpcInterface_h_
#define MblCloudConnectIpcInterface_h_

#include <vector>

#include "MblError.h"
#include "MblCloudConnectTypes.h"

namespace mbl {

/**
 * @brief Mbl Cloud Connect Ipc Interface.
 * This class provides an interface for all IPC mechanisms that allow IPC comminication between CCRB and client applications.
 */
class MblCloudConnectIpcInterface {

public:

    MblCloudConnectIpcInterface() = default;
    virtual ~MblCloudConnectIpcInterface() = default;

/**
 * @brief Initializes IPC mechanism.
 * 
 * @return MblError returns value Error::None if function succeeded, or error code otherwise.
 */
    virtual MblError init() = 0;

/**
 * @brief Deinitializes IPC mechanism.
 * 
 * @return MblError returns value Error::None if function succeeded, or error code otherwise.
 */
    virtual MblError de_init() = 0;

/**
 * @brief Runs IPC event-loop.
 * 
 * @return MblError returns value Error::None if function succeeded, or error code otherwise.
 */
    virtual MblError run() = 0;

/**
 * @brief Stops IPC event-loop.
 * 
 * @return MblError returns value Error::None if function succeeded, or error code otherwise.
 */
    virtual MblError stop() = 0;

/**
 * @brief Sends registration request final status to the destination client application. 
 * This function sends a final status of the registration request, that was initiated 
 * by a client application via calling register_resources_async API. 
 * @param ipc_conn_handle is a handle to the IPC unique connection information of the application 
 *        that should be notified.
 * @param access_token is a token that should be used by the client application in all APIs that 
 *        access (in any way) to the registered set of resources. Value of this argument 
 *        should be used by the client application only if registration was successfull 
 *        for all resources.
 * @param reg_status FIXME
 * @return MblError returns Error::None if the message was successfully delivered, 
 *         or error code otherwise. 
 */
virtual MblError update_registration_status(
        const uintptr_t ipc_conn_handle, 
        const std::string &access_token,
        const MblError reg_status) = 0;

/**
 * @brief Sends deregistration request final status to the destination client application. 
 * This function sends a final status of the deregistration request, that was initiated 
 * by a client application via calling deregister_resources_async API. 
 * @param ipc_conn_handle is a handle to the IPC unique connection information of the application 
 *        that should be notified.
 * @param dereg_status FIXME
 * @return MblError returns Error::None if the message was successfully delivered, 
 *         or error code otherwise. 
 */
virtual MblError update_deregistration_status(
        const uintptr_t ipc_conn_handle, 
        const MblError dereg_status) = 0;

/**
 * @brief Sends resource instances addition request final status to the destination client application. 
 * This function sends a final status of the resource instances addition request, that was initiated 
 * by a client application via calling add_resource_instances_async API. 
 * @param ipc_conn_handle is a handle to the IPC unique connection information of the application 
 *        that should be notified.
 * @param add_status FIXME
 * @return MblError returns Error::None if the message was successfully delivered, 
 *         or error code otherwise. 
 */
virtual MblError update_add_resource_instance_status(
        const uintptr_t ipc_conn_handle, 
        const MblError add_status) = 0;

/**
 * @brief Sends resource instances removal request final status to the destination client application. 
 * This function sends a final status of the resource instances removal request, that was initiated 
 * by a client application via calling remove_resource_instances_async API. 
 * @param ipc_conn_handle is a handle to the IPC unique connection information of the application 
 *        that should be notified.
 * @param remove_status FIXME
 * @return MblError returns Error::None if the message was successfully delivered, 
 *         or error code otherwise. 
 */
virtual MblError update_remove_resource_instance_status(
        const uintptr_t ipc_conn_handle, 
        const MblError remove_status) = 0;
 
private:

    // No copying or moving (see https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#cdefop-default-operations)
    MblCloudConnectIpcInterface(const MblCloudConnectIpcInterface&) = delete;
    MblCloudConnectIpcInterface & operator = (const MblCloudConnectIpcInterface&) = delete;
    MblCloudConnectIpcInterface(MblCloudConnectIpcInterface&&) = delete;
    MblCloudConnectIpcInterface& operator = (MblCloudConnectIpcInterface&&) = delete;    
};

} // namespace mbl

#endif // MblCloudConnectIpcInterface_h_
