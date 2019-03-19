/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DBusMessage_h_
#define _DBusMessage_h_

#include "CloudConnectTypes.h"

#include <systemd/sd-bus.h>

#include <string>

namespace mbl
{

class ResourceBroker;

/**
 * @brief Implements pure virtual base class for processing and replying on messages received
 * from sd-bus.
 * Message processing includes message parse, call correspondent ccrb API and sends
 * method reply / error reply to sender.
 */
class DBusCommonMessageProcessor
{
public:
    DBusCommonMessageProcessor(const std::string message_format,
                               const std::string reply_message_format);

    virtual ~DBusCommonMessageProcessor() {}

    /**
     * @brief D-Bus message process - pure virtual function implemented by the processors of
     * specific messages.
     * @param connection_handle - sd-bus bus connection handle
     * @param m - message received on sd-bus bus
     * @param ccrb - reference to the Cloud Connect resource broker
     * @param ret_error - The sd_bus_error structure carries information about a D-Bus error
     * condition.
     * @return int - 0 on success, On failure, return a negative Linux errno-style error code.
     */
    virtual int process_message(sd_bus* connection_handle,
                                sd_bus_message* m,
                                ResourceBroker& ccrb,
                                sd_bus_error* ret_error) = 0;

protected:
    /**
     * @brief Verify received on sd-bus message signature. The signature should match with the
     * interface defined in sd_bus_vtable.
     * @param ret_error - The sd_bus_error structure carries information about a D-Bus error
     * condition.
     * @return int - 0 on success, On failure, return a negative Linux errno-style error code.
     */
    int verify_signature(sd_bus_message* m, sd_bus_error* ret_error);

    /**
     * @brief Reply on sd-bus message: construct and send a reply to the message sender.
     * @param connection_handle - sd-bus bus connection handle
     * @param m_to_reply_on - sd-bus message to reply on
     * @param ret_error - The sd_bus_error structure carries information about a D-Bus error
     * condition.
     * @return int - 0 on success, On failure, return a negative Linux errno-style error code.
     */
    virtual int reply_on_message(sd_bus* connection_handle,
                                 sd_bus_message* m_to_reply_on,
                                 sd_bus_error* ret_error,
                                 CloudConnectStatus status);

    /**
     * @brief Read SD_BUS_TYPE_STRING argument from received message.
     *
     * @param m message that encapsulates method invocation whose signature is verified.
     * @param ret_error sd-bus error description place-holder that is filled in the
     *                  case of an error.
     * @return std pair of int sd-bus error number of the error,
     *         or 0 in case of success and string that was read from the message
     */
    std::pair<int, std::string> get_string_argument(sd_bus_message* m, sd_bus_error* ret_error);

    /**
     * @brief Handles ResourceBroker RegisterResources / DeregisterResources method failure.
     * Fills ret_error according to mbl_status and cc_status returned
     * from ResourceBroker method. ret_error returned to sd-bus event loop, and that
     * causes sending reply-error to to the application.
     *
     * @param mbl_status MblError error code tha was returned from ResourceBroker method.
     * @param cc_status Cloud Connect error code tha was returned from ResourceBroker method.
     * @param method_name ResourceBroker method name that failed.
     * @param ret_error sd-bus error description place-holder that is filled with
     *                  appropriate error.
     * @return int Cloud Connect error (always negative value). The value returned is the
     *             negative value of input cc_status, or ERR_INTERNAL_ERROR.
     */
    int handle_message_process_failure(CloudConnectStatus cc_status,
                                       const char* method_name,
                                       sd_bus_error* ret_error);

    // message format
    const std::string message_format_;
    // message reply format
    const std::string reply_message_format_;

private:
    /**
     * @brief Append data to the reply message - pure virtual function implemented by
     * processors of specific messages.
     * @param m_reply - reply message
     * @param member - member field from message header
     * @param status - Cloud Connect return status
     * @param ret_error - The sd_bus_error structure carries information about a D-Bus error
     * condition.
     * @return int - 0 on success, On failure, return a negative Linux errno-style error code.
     */
    virtual int fill_reply_message(sd_bus_message* m_reply,
                                   const std::string& member,
                                   CloudConnectStatus status,
                                   sd_bus_error* ret_error) = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Implements processing of register resources message.
 * Message processing includes message parcing, call correspondent ccrb API and sends
 * method reply / error reply to the sender.
 */
class DBusRegisterResourcesMessageProcessor : public DBusCommonMessageProcessor
{
public:
    DBusRegisterResourcesMessageProcessor();

    /**
     * @brief Process register resources message and sends method reply / error reply to sender.
     * @param connection_handle - sd-bus bus connection handle
     * @param m - message received on sd-bus bus
     * @param ccrb - reference to the Cloud Connect resource broker
     * @param ret_error - The sd_bus_error structure carries information about a D-Bus error
     * condition.
     * @return int - 0 on success, On failure, return a negative Linux errno-style error code.
     */
    virtual int process_message(sd_bus* connection_handle,
                                sd_bus_message* m,
                                ResourceBroker& ccrb,
                                sd_bus_error* ret_error);

protected:
    // access token
    std::string access_token_;

private:
    /**
     * @brief Append access token to the register resources reply message.
     * @param m_reply - reply message
     * @param member - member field from message header
     * @param status - Cloud Connect return status
     * @param ret_error - The sd_bus_error structure carries information about a D-Bus error
     * condition.
     * @return int - 0 on success, On failure, return a negative Linux errno-style error code.
     */
    virtual int fill_reply_message(sd_bus_message* m_reply,
                                   const std::string& member,
                                   CloudConnectStatus status,
                                   sd_bus_error* ret_error);
};

/**
 * @brief Implements processing for deregister resources message.
 * Message processing includes message parcing, call correspondent ccrb API and sends
 * method reply / error reply to the sender.
 */
class DBusDeregisterResourcesMessageProcessor : public DBusCommonMessageProcessor
{
public:
    DBusDeregisterResourcesMessageProcessor();

    /**
     * @brief Process deregister resources message and sends method reply / error reply to sender.
     * @param connection_handle - sd-bus bus connection handle
     * @param m - message received on sd-bus bus
     * @param ccrb - reference to the Cloud Connect resource broker
     * @param ret_error - The sd_bus_error structure carries information about a D-Bus error
     * condition.
     * @return int - 0 on success, On failure, return a negative Linux errno-style error code.
     */
    virtual int process_message(sd_bus* connection_handle,
                                sd_bus_message* m,
                                ResourceBroker& ccrb,
                                sd_bus_error* ret_error);

private:
    /**
     * @brief Append Cloud Connect return status of deregister resources operation to the reply
     * message.
     * @param m_reply - reply message
     * @param member - member field from message header
     * @param status - Cloud Connect return status
     * @param ret_error - The sd_bus_error structure carries information about a D-Bus error
     * condition.
     * @return int - 0 on success, On failure, return a negative Linux errno-style error code.
     */
    virtual int fill_reply_message(sd_bus_message* m_reply,
                                   const std::string& member,
                                   CloudConnectStatus status,
                                   sd_bus_error* ret_error);
};

/**
 * @brief Implements processing for set resources values message.
 * Message processing includes message parcing, call correspondent ccrb API and sends
 * method reply / error reply to the sender.
 */
class DBusSetResourcesMessageProcessor : public DBusCommonMessageProcessor
{
public:
    DBusSetResourcesMessageProcessor();

    /**
     * @brief Process set LWM2M resources values message and sends method reply / error reply
     * to sender.
     * @param connection_handle - sd-bus bus connection handle
     * @param m - message received on sd-bus bus
     * @param ccrb - reference to the Cloud Connect resource broker
     * @param ret_error - The sd_bus_error structure carries information about a D-Bus error
     * condition.
     * @return int - 0 on success, On failure, return a negative Linux errno-style error code.
     */
    virtual int process_message(sd_bus* connection_handle,
                                sd_bus_message* m,
                                ResourceBroker& ccrb,
                                sd_bus_error* ret_error);

    /**
     * @brief Handles ResourceBroker SetResourcesValues method failure.
     * Fills ret_error according to set_operations and cc_status returned from ResourceBroker
     * method. ret_error returned to sd-bus event loop and that causes sending reply-error
     * to the application.
     *
     * @param set_operations vector containing set operations data and Cloud Connect status
     * code related to the specific set operation as it was returned from ResourceBroker method.
     * @param cc_status Cloud Connect error code tha was returned from ResourceBroker method.
     * @param ret_error sd-bus error description place-holder that is filled with
     *                  appropriate error.
     * @return int Cloud Connect error (always negative value). The value returned is the
     *             negative value of input cc_status, or ERR_INTERNAL_ERROR.
     */
    int handle_message_process_failure(sd_bus_message* m,
                                       std::vector<ResourceSetOperation>& set_operations,
                                       CloudConnectStatus cc_status,
                                       sd_bus_error* ret_error);

private:
    /**
     * @brief Reads resource array argument from the incoming message
     * @param m - message received on sd-bus bus
     * @param resource_set_operation - vector containing set request information
     * @param ret_error - The sd_bus_error structure carries information about a D-Bus error
     * condition.
     * @return int - 0 on success, On failure, return a negative Linux errno-style error code.
     */
    int read_array_from_message(sd_bus_message* m,
                                std::vector<ResourceSetOperation>& resource_set_operation,
                                sd_bus_error* ret_error);
    /**
     * @brief Set resources reply is an empty message.
     * @param m_reply - reply message
     * @param member - member field from message header
     * @param status - Cloud Connect return status
     * @param ret_error - The sd_bus_error structure carries information about a D-Bus error
     *  condition.
     * @return int - 0 on success, On failure, return a negative Linux errno-style error code.
     */
    virtual int fill_reply_message(sd_bus_message* m_reply,
                                   const std::string& member,
                                   CloudConnectStatus status,
                                   sd_bus_error* ret_error);

    /**
     * @brief Iterates over the set resources values operation reply and checks if there is at
     * least one error.
     * @param status - Cloud Connect return status of SetResourcesValues operation
     * @param set_operations - vector containing set resources values operation
     * @return bool - true if Cloud Connect status is not STATUS_SUCCESS or set_operations vector
     * contains at least one error, false otherwise.
     */
    bool is_message_process_failure(CloudConnectStatus& status,
                                    std::vector<ResourceSetOperation>& set_operations);
};

/**
 * @brief Implements processing for get resources values message.
 * Message processing includes message parcing, call correspondent ccrb API and sends
 * method reply / error reply to the sender.
 */
class DBusGetResourcesMessageProcessor : public DBusCommonMessageProcessor
{
public:
    DBusGetResourcesMessageProcessor();

    /**
     * @brief Process get LWM2M resources values message and sends method reply / error reply
     * to sender.
     * @param connection_handle - sd-bus bus connection handle
     * @param m - message received on sd-bus bus
     * @param ccrb - reference to the Cloud Connect resource broker
     * @param ret_error - The sd_bus_error structure carries information about a D-Bus error
     * condition.
     * @return int - 0 on success, On failure, return a negative Linux errno-style error code.
     */
    virtual int process_message(sd_bus* connection_handle,
                                sd_bus_message* m,
                                ResourceBroker& ccrb,
                                sd_bus_error* ret_error);

    /**
     * @brief Handles ResourceBroker GetResourcesValues method failure.
     * Fills ret_error according to get_operations and cc_status returned
     * from ResourceBroker method. ret_error returned to sd-bus event loop and that
     * causes sending reply-error to the application.
     * If cc_status is an error, sd-bus ret_error will be set to the value of cc_status.
     * If there are one or more errors related to specific paths, sd-bus ret_error will be set
     * to the value of one of the errors, also 10 first failed resource paths and there
     * correspondent error statuses will be listed in reply-error.
     * @param m - message received on sd-bus bus
     * @param get_operations vector containing get operations data and Cloud Connect status
     * code related to the specific get operation as it was returned from ResourceBroker method.
     * @param cc_status Cloud Connect error code that was returned from ResourceBroker method.
     * cc_status is related to all resources listed in this get operations, for example
     * ERR_INVALID_ACCESS_TOKEN.
     * @param ret_error sd-bus error description place-holder that is filled with
     *                  appropriate error.
     * @return int Cloud Connect error (always negative value). The value returned is the
     *             negative value of input cc_status, or ERR_INTERNAL_ERROR.
     */
    int handle_message_process_failure(sd_bus_message* m,
                                       std::vector<ResourceGetOperation>& get_operations,
                                       CloudConnectStatus cc_status,
                                       sd_bus_error* ret_error);

private:
    /**
     * @brief Reads resource array argument from the incoming message
     * @param m - message received on sd-bus bus
     * @param resource_get_operation - vector containing get request information
     * @param ret_error - The sd_bus_error structure carries information about a D-Bus error
     * condition.
     * @return int - 0 on success, On failure, return a negative Linux errno-style error code.
     */
    int read_array_from_message(sd_bus_message* m,
                                std::vector<ResourceGetOperation>& resource_get_operation,
                                sd_bus_error* ret_error);

    /**
     * @brief Append array of data type and data value pairs to the reply message.
     * @param m_reply - reply message
     * @param member - member field from message header
     * @param status - Cloud Connect return status
     * @param ret_error - The sd_bus_error structure carries information about a D-Bus error
     * condition.
     * @return int - 0 on success, On failure, return a negative Linux errno-style error code.
     */
    virtual int fill_reply_message(sd_bus_message* m_reply,
                                   const std::string& member,
                                   CloudConnectStatus status,
                                   sd_bus_error* ret_error);

    /**
     * @brief Iterates over the set resources values operation reply and checks if there is at
     * least one error.
     * @param status - Cloud Connect return status of GetResourcesValues operation
     * @param get_operations - vector containing get resources values operation
     * @return bool - true if Cloud Connect status is not STATUS_SUCCESS or get_operations vector
     * contains at least one error, false otherwise.
     */
    bool is_message_process_failure(CloudConnectStatus& status,
                                    std::vector<ResourceGetOperation>& get_operations);

    // resource get operation vector
    std::vector<ResourceGetOperation> get_operations_;
};

} // namespace mbl {

#endif //_DBusMessage_h_
