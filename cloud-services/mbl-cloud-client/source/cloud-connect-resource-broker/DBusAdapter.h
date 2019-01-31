/*
 * Copyright (c) 2016-2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: ...
 */

#ifndef DBusAdapter_h_
#define DBusAdapter_h_

#include <pthread.h>

#include "MblError.h"

namespace mbl {

class ResourceBroker;

/**
 * @brief Implements an interface to the D-Bus IPC.
 * This class provides an implementation for the handlers that 
 * will allow comminication between Pelion Cloud Connect D-Bus 
 * service and client applications.
 */
class DBusAdapter {

public:

    DBusAdapter(ResourceBroker &ccrb);
    ~DBusAdapter();

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
// FIXME DBusAdapter


private:
   
    // Temporary solution for exiting from simulated event-loop that should be removed after we introduce real sd-bus event-loop.
    // Now we just use this boolean flag, that signals, that the thread should exit from simulated event-loop.
    // In future we shall replace this flag with real mechanism, that will allow exiting from real sd-bus event-loop.
    volatile bool exit_loop_;

    // this class must have a reference that should be always valid to the CCRB instance. 
    // reference class member satisfy this condition.   
    ResourceBroker &ccrb_;

    // No copying or moving (see https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#cdefop-default-operations)
    DBusAdapter(const DBusAdapter&) = delete;
    DBusAdapter & operator = (const DBusAdapter&) = delete;
    DBusAdapter(DBusAdapter&&) = delete;
    DBusAdapter& operator = (DBusAdapter&&) = delete;    
};

} // namespace mbl

#endif // DBusAdapter_h_
