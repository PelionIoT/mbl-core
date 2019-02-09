/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DBusAdapter_h_
#define DBusAdapter_h_


#include <inttypes.h>
#include <memory>

#include "MblError.h"

// to be used by Google Test testing
class DBusAdapterTester;

namespace mbl {

class ResourceBroker;

// FIXME : defined temporarily - remove later
enum CloudConnectStatus 
{           //very important - will be used by dbus to send error/reply message
    CLOUD_CONNECT_STATUS_SUCCESS,    // negative values are failure
     CLOUD_CONNECT_STATUS_FAILURE,   // positive or 0  values are success
};


/**
 * @brief Implements an interface to the D-Bus IPC.
 * This class provides an implementation for the handlers that 
 * will allow comminication between Pelion Cloud Connect D-Bus 
 * service and client applications.
 */
class DBusAdapter {
friend class ::DBusAdapterTester;
public:    
    //TODO: fix gtest issue
    DBusAdapter();
    //DBusAdapter(ResourceBroker &ccrb);
    ~DBusAdapter();   //TODO- moved to cpp remove this from here

/**
 * @brief Initializes IPC mechanism.
 * 
 * @return MblError returns value Error::None if function succeeded, 
 *         or error code otherwise.
 */
    MblError init();

/**
 * @brief Deinitializes IPC mechanism.
 * 
 * @return MblError returns value Error::None if function succeeded, 
 *         or error code otherwise.
 */
    MblError deinit();

/**
 * @brief Runs IPC event-loop.
 * @param stop_status is used in order to understand the reason for stoping the adapter
 * This can be due to calling stop() , or due to other reasons
 * @return MblError returns value Error::None if function succeeded, 
 *         or error code otherwise.
 */
    MblError run(MblError &stop_status);

/**
 * @brief Stops IPC event-loop.
 * @param stop_status is used in order to understand the reason for stoping the adapter
 *  use MblError::DBUS_ADAPTER_STOP_STATUS_NO_ERROR by default (no error)
 * @return MblError returns value Error::None if function succeeded, 
 *         or error code otherwise.
 */
    MblError stop(MblError stop_status);

/**
 * @brief Sends registration request final status to the destination 
 * client application. 
 * This function sends a final status of the registration request, 
 * that was initiated by a client application via calling 
 * register_resources_async API. 
 * @param ipc_conn_handle is a handle to the IPC unique connection information 
 *        of the application that should be notified.
 * @param access_token is a token that should be used by the client application
 *        in all APIs that access (in any way) to the registered set of 
 *        resources. Value of this argument should be used by the client 
 *        application only if registration was successfull for all resources.
 * @param reg_status status of registration of all resources. 
 *        reg_status is SUCCESS only if registration of all resources was 
 *        successfully finished, or error code otherwise.
 * @return MblError returns Error::None if the message was successfully 
 *         delivered, or error code otherwise. 
 */
    MblError update_registration_status(
        const uintptr_t ipc_conn_handle, 
        const std::string &access_token,
        const CloudConnectStatus reg_status);

/**
 * @brief Sends deregistration request final status to the destination client 
 * application. This function sends a final status of the deregistration 
 * request, that was initiated by a client application via calling 
 * deregister_resources_async API. 
 * @param ipc_conn_handle is a handle to the IPC unique connection information
 *        of the application that should be notified.
 * @param dereg_status status of deregistration of all resources. 
 *        dereg_status is SUCCESS only if deregistration of all resources was 
 *        successfully finished, or error code otherwise.
 * @return MblError returns Error::None if the message was successfully delivered, 
 *         or error code otherwise. 
 */
    MblError update_deregistration_status(
        const uintptr_t ipc_conn_handle, 
        const CloudConnectStatus dereg_status);

/**
 * @brief Sends resource instances addition request final status to the destination
 * client application. This function sends a final status of the resource instances
 * addition request, that was initiated by a client application via calling 
 * add_resource_instances_async API. 
 * @param ipc_conn_handle is a handle to the IPC unique connection information of 
 *        the application that should be notified.
 * @param add_status status of resource instances addition. 
 *        add_status is SUCCESS only if the addition of all resources instances was 
 *        successfully finished, or error code otherwise.
 * @return MblError returns Error::None if the message was successfully delivered, 
 *         or error code otherwise. 
 */
    MblError update_add_resource_instance_status(
        const uintptr_t ipc_conn_handle, 
        const CloudConnectStatus add_status);

/**
 * @brief Sends resource instances removal request final status to the destination
 * client application. This function sends a final status of the resource instances
 * removal request, that was initiated by a client application via calling 
 * remove_resource_instances_async API. 
 * @param ipc_conn_handle is a handle to the IPC unique connection information 
 *        of the application that should be notified.
 * @param remove_status status of resource instances removal. 
 *        remove_status is SUCCESS only if the removal of all resources instances was 
 *        successfully finished, or error code otherwise.
 * @return MblError returns Error::None if the message was successfully delivered, 
 *         or error code otherwise. 
 */
    MblError update_remove_resource_instance_status(
        const uintptr_t ipc_conn_handle, 
        const CloudConnectStatus remove_status);


private:
    // forward declaration - PIMPL implementation class - defined in cpp source file
    // unique pointer impl_ will automatically  destroy the object
    class DBusAdapterImpl;
    std::unique_ptr<DBusAdapterImpl> impl_;
      
    // No copying or moving 
    // (see https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#cdefop-default-operations)
    DBusAdapter(const DBusAdapter&) = delete;
    DBusAdapter & operator = (const DBusAdapter&) = delete;
    DBusAdapter(DBusAdapter&&) = delete;
    DBusAdapter& operator = (DBusAdapter&&) = delete;    
};

} // namespace mbl

#endif // DBusAdapter_h_
