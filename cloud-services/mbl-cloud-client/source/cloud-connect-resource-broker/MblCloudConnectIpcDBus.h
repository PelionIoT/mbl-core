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


#ifndef MblCloudConnectIpcDBus_h_
#define MblCloudConnectIpcDBus_h_

#include <pthread.h>

namespace mbl {

class MblCloudConnectResourceBroker;

/**
 * @brief Implements an interface to the D-Bus IPC.
 * This class provides an implementation for the handlers that 
 * will allow comminication between Pelion Cloud Connect D-Bus 
 * service and client applications.
 */
class MblCloudConnectIpcDBus {

public:

    MblCloudConnectIpcDBus(MblCloudConnectResourceBroker &ccrb);
    ~MblCloudConnectIpcDBus() override;

/**
 * @brief Initializes IPC mechanism.
 * 
 * @return MblError returns value Error::None if function succeeded, or error code otherwise.
 */
    MblError init();

/**
 * @brief Deinitializes IPC mechanism.
 * 
 * @return MblError returns value Error::None if function succeeded, or error code otherwise.
 */
    MblError de_init();

/**
 * @brief Runs IPC event-loop.
 * 
 * @return MblError returns value Error::None if function succeeded, or error code otherwise.
 */
    MblError run();

/**
 * @brief Stops IPC event-loop.
 * 
 * @return MblError returns value Error::None if function succeeded, or error code otherwise.
 */
    MblError stop();

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
    MblError update_registration_status(
        const uintptr_t ipc_conn_handle, 
        const std::string &access_token,
        const MblError reg_status);

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
        const MblError dereg_status);

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
        const MblError add_status);

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
        const MblError remove_status);

// FIX return MblError description !!!!!!!!!!!!!!!!!!!!!!!!!

// FIX ALL FIXME!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


private:
   
    // Temporary solution for exiting from simulated event-loop that should be removed after we introduce real sd-bus event-loop.
    // Now we just use this boolean flag, that signals, that the thread should exit from simulated event-loop.
    // In future we shall replace this flag with real mechanism, that will allow exiting from real sd-bus event-loop.
    volatile bool exit_loop_;

    // this class must have a reference that should be always valid to the CCRB instance. 
    // reference class member satisfy this condition.   
    MblCloudConnectResourceBroker &ccrb_;

    // No copying or moving (see https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#cdefop-default-operations)
    MblCloudConnectIpcDBus(const MblCloudConnectIpcDBus&) = delete;
    MblCloudConnectIpcDBus & operator = (const MblCloudConnectIpcDBus&) = delete;
    MblCloudConnectIpcDBus(MblCloudConnectIpcDBus&&) = delete;
    MblCloudConnectIpcDBus& operator = (MblCloudConnectIpcDBus&&) = delete;    
};

} // namespace mbl

#endif // MblCloudConnectIpcDBus_h_
