/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DBusAdapter_h_
#define DBusAdapter_h_

#include "CloudConnectExternalTypes.h"
#include "Event.h"
#include "MblError.h"

#include <inttypes.h>
#include <memory>
#include <string>

// to be used by Google Test testing
class TestInfra_DBusAdapterTester;

namespace mbl
{

class ResourceBroker;
class DBusAdapterImpl;
class IpcConnection;

/**
 * @brief Implements an interface to the D-Bus IPC.
 * This class provides an implementation for the handlers that
 * will allow comminication between Pelion Cloud Connect D-Bus
 * service and client applications.
 */
class DBusAdapter
{
    // Google test friend class (for tests to be able to reach private members)
    friend class ::TestInfra_DBusAdapterTester;

public:
    DBusAdapter(ResourceBroker& ccrb);

    // dtor must be explicitly declared and get default implementation in source in order to allow
    // member impl_ unique_ptr to reqcognize DBusAdapterImpl as a complete type and deestroy it
    virtual ~DBusAdapter();

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
    MblError run(MblError& stop_status);

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
     * register_resources API.
     * @param destination represents the unique IPC connection information
     *        of the application that should be notified.
     * @param reg_status status of registration of all resources.
     *        reg_status is SUCCESS only if registration of all resources was
     *        successfully finished, or error code otherwise.
     *
     * @return MblError returns Error::None if the message was successfully
     *         delivered, or error code otherwise.
     */
    virtual MblError update_registration_status(const IpcConnection& destination,
                                                const CloudConnectStatus reg_status);

    /**
     * @brief Sends deregistration request final status to the destination client
     * application. This function sends a final status of the deregistration
     * request, that was initiated by a client application via calling
     * deregister_resources API.
     * @param destination represents the unique IPC connection information
     *        of the application that should be notified.
     * @param dereg_status status of deregistration of all resources.
     *        dereg_status is SUCCESS only if deregistration of all resources was
     *        successfully finished, or error code otherwise.
     * @return MblError returns Error::None if the message was successfully delivered,
     *         or error code otherwise.
     */
    MblError update_deregistration_status(const IpcConnection& destination,
                                          const CloudConnectStatus dereg_status);

    /**
     * @brief
     * Send 'deferred event' to the event loop using
     * sd_event_add_defer(). Must be called by CCRB thread only.
     * For more details: https://www.freedesktop.org/software/systemd/man/sd_event_add_defer.html#
     * @param data - the data to be sent. This can be any Plain Old Type (POD)
     * structure or native type.To transfer a pointer, wrap it in a struct on stuck and use the
     * struct. Any attempt to use non-POD type will fail compilation (static_assert).
     * @param data_length - length of data used, must be less than the maximum size allowed
     * (sizeof(T))
     * @param callback - callback to be called when event is fired
     * @param description - optional - description for the event cause, can leave empty string or
     * not supply at all
     * @return pair - Error::None and generated event id for success, otherwise the failure reason
     * and UINTMAX_MAX
     */
    template <typename T>
    std::pair<MblError, uint64_t> send_event_immediate(T& data,
                                                       unsigned long data_length,
                                                       Event::UserCallback callback,
                                                       const std::string& description = "");

    /**
     * @brief
     *
     * Send 'timed event' to the event loop using sd_event_add_time(). Must be called by CCRB
     * thread only.  CLOCK_MONOTONIC is specified for sd_event_add_time() clock identifier.
     * sd_event_add_time() passed the default accuracy parameter equal to 0. The accuracy
     * parameter specifies an additional accuracy value in µs specifying how much the timer event
     * may be delayed. Accuracy 0 to selects the default accuracy (250ms).
     * For more details: https://www.freedesktop.org/software/systemd/man/sd_event_add_time.html#
     * @param data - the data to be sent. This can be any Plain Old Type (POD)
     * structure or native type.To transfer a pointer, wrap it in a struct on stuck and use the
     * struct. Any attempt to use non-POD type will fail compilation (static_assert).
     * @param data_length - length of data used, must be less than the maximum size allowed
     * @param callback - callback to be called when event is fired
     * @param period_millisec  - period in miliseconds after which event will be sent. The event
     * will
     * continue to be sent periodically after each period time. Minimal value for period_millisec
     * is 100 milliseconds.
     * @param description - optional - description for the event cause, can leave empty string or
     * not supply at all
     * @return pair - Error::None and generated event id for success, otherwise the failure reason
     * and UINTMAX_MAX
     */
    template <typename T>
    std::pair<MblError, uint64_t> send_event_periodic(T& data,
                                                      unsigned long data_length,
                                                      Event::UserCallback callback,
                                                      uint64_t period_millisec,
                                                      const std::string& description = "");

    /**
    * @brief Generate unique access token using sd_id128_randomize
    *
    * @return std::pair<MblError, std::string> - a pair where the first element Error::None for
    * success, therwise the failure reason.
    * If the first element is Error::None, user may access the second element which is the
    * generate access token. most compilers will optimize (move and not copy). If first element is
    * not success - user should ignore the second element.
    */
    std::pair<MblError, std::string> generate_access_token();

private:
    // forward declaration - PIMPL implementation class - defined in cpp source file
    // unique pointer impl_ will automatically  destroy the object
    std::unique_ptr<DBusAdapterImpl> impl_;

    // No copying or moving
    // (see:
    // https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#cdefop-default-operations)
    DBusAdapter(const DBusAdapter&) = delete;
    DBusAdapter& operator=(const DBusAdapter&) = delete;
    DBusAdapter(DBusAdapter&&) = delete;
    DBusAdapter& operator=(DBusAdapter&&) = delete;
};

} // namespace mbl

#endif // DBusAdapter_h_
