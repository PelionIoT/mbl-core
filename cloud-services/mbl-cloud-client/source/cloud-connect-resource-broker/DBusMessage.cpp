/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "DBusMessage.h"
#include "CloudConnectTrace.h"
#include "CloudConnectTypes.h"
#include "DBusAdapterCommon.h"
#include "DBusCloudConnectNames.h"
#include "ResourceBroker.h"

#include <sstream>
#include <string>

#define TRACE_GROUP "ccrb-dbus"
// maximal size of formatted error message that is sent to the caller
#define SD_BUS_RETURN_ERROR_MESSAGE_MAX_LENGTH 500
// maximal number of failed get/set resource values in formatted error message
#define MAX_NUMBER_OF_ERRORS_IN_REPLY 10

namespace mbl
{

DBusCommonMessageProcessor::DBusCommonMessageProcessor(const std::string message_signature,
                                                       const std::string reply_message_signature)
    : message_signature_(message_signature), reply_message_signature_(reply_message_signature)
{
    TR_DEBUG_ENTER;
}

int DBusCommonMessageProcessor::verify_signature(sd_bus_message* m, sd_bus_error* ret_error)
{
    TR_DEBUG_ENTER;
    assert(m);
    assert(ret_error);

    if (0 >= sd_bus_message_has_signature(m, message_signature_.c_str())) {

        std::string signature = sd_bus_message_get_signature(m, 1);

        std::stringstream msg("Unexpected message signature: " + signature +
                              ", expected message signature: " + message_signature_ + ", member " +
                              std::string(sd_bus_message_get_member(m)) + ", sender " +
                              std::string(sd_bus_message_get_sender(m)));
        return LOG_AND_SET_SD_BUS_ERROR_F(ENOMSG, ret_error, msg);
    }

    return 0;
}

std::pair<int, std::string> DBusCommonMessageProcessor::get_string_argument(sd_bus_message* m,
                                                                            sd_bus_error* ret_error)
{
    TR_DEBUG_ENTER;
    assert(m);
    assert(ret_error);

    const char* out_read = nullptr;
    int r = sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &out_read);
    if (r < 0) {
        std::stringstream msg("Member " + std::string(sd_bus_message_get_member(m)) + ", sender " +
                              std::string(sd_bus_message_get_sender(m)) +
                              " : sd_bus_message_read_basic SD_BUS_TYPE_STRING");
        return std::pair<int, std::string>(LOG_AND_SET_SD_BUS_ERROR_F(r, ret_error, msg), "");
    }

    if ((nullptr == out_read) || (strlen(out_read) == 0)) {
        std::stringstream msg("sd_bus_message_read_basic empty string! Member: " +
                              std::string(sd_bus_message_get_member(m)) + ", sender: " +
                              std::string(sd_bus_message_get_sender(m)));
        return std::pair<int, std::string>(LOG_AND_SET_SD_BUS_ERROR_F(EINVAL, ret_error, msg), "");
    }

    return std::pair<int, std::string>(0, std::string(out_read));
}

int DBusCommonMessageProcessor::reply_on_message(sd_bus* connection_handle,
                                                 sd_bus_message* m_to_reply_on,
                                                 sd_bus_error* ret_error,
                                                 CloudConnectStatus status)
{
    TR_DEBUG_ENTER;
    assert(connection_handle);
    assert(m_to_reply_on);
    assert(ret_error);

    const char* method_name = sd_bus_message_get_member(m_to_reply_on);
    assert(method_name);
    const char* sender_name = sd_bus_message_get_sender(m_to_reply_on);
    assert(sender_name);
    TR_INFO("Sending reply on %s method to %s", method_name, sender_name);

    sd_bus_message* m_reply = nullptr;

    // When we leave this function scope, we need to call sd_bus_message_unrefp on m_reply
    sd_bus_object_cleaner<sd_bus_message> reply_cleaner(m_reply, sd_bus_message_unref);

    int r = sd_bus_message_new_method_return(m_to_reply_on, &m_reply);
    if (r < 0) {
        std::stringstream msg("Sending reply on " + std::string(method_name) + ", sender name " +
                              std::string(sender_name) +
                              " : sd_bus_message_new_method_return error");
        return LOG_AND_SET_SD_BUS_ERROR_F(r, ret_error, msg);
    }

    // append data to the reply message
    r = fill_reply_message(m_reply, method_name, status, ret_error);
    if (r < 0) {
        std::stringstream msg("Sending reply on " + std::string(method_name) + ", sender name " +
                              std::string(sender_name) + " : fill_reply_message error");
        return LOG_AND_SET_SD_BUS_ERROR_F(r, ret_error, msg);
    }
    // send the message
    r = sd_bus_send(connection_handle, m_reply, nullptr);
    if (r < 0) {
        std::stringstream msg("Sending reply on " + std::string(method_name) + ", sender name " +
                              std::string(sender_name) + " : sd_bus_send error");
        return LOG_AND_SET_SD_BUS_ERROR_F(r, ret_error, msg);
    }

    TR_INFO("Reply on %s method successfully sent to %s", method_name, sender_name);

    return 0;
}

int DBusCommonMessageProcessor::handle_message_process_failure(CloudConnectStatus cc_status,
                                                               const char* method_name,
                                                               sd_bus_error* ret_error)
{
    TR_DEBUG_ENTER;

    assert(is_cloud_connect_error(cc_status));
    assert(method_name);
    assert(ret_error);

    // we have cloud connect related error in resource broker
    TR_ERR(
        "%s failed with cloud connect error %s", method_name, CloudConnectStatus_to_str(cc_status));

    // set custom error to sd_bus_error structure.

    // sd_bus_error_set_const translates DBus format error string to negative integer
    // and returns it's value(in this case -status_to_send)
    int r = sd_bus_error_set_const(
        ret_error,
        CloudConnectStatus_error_to_DBus_format_str(cc_status), // sd_bus_error.name
        CloudConnectStatus_to_readable_str(cc_status));         // sd_bus_error.message
    assert(-cc_status == r);
    return r;
}

/***************************   DBusRegisterResourcesMessageProcessor  ****************************/

DBusRegisterResourcesMessageProcessor::DBusRegisterResourcesMessageProcessor()
    : DBusCommonMessageProcessor("s", "s")
{
    TR_DEBUG_ENTER;
}

int DBusRegisterResourcesMessageProcessor::process_message(sd_bus* connection_handle,
                                                           sd_bus_message* m,
                                                           ResourceBroker& ccrb,
                                                           sd_bus_error* ret_error)
{
    TR_DEBUG_ENTER;
    assert(connection_handle);
    assert(m);
    assert(ret_error);

    // verify the signature
    int r = verify_signature(m, ret_error);
    if (r < 0) {
        TR_ERR("verify_signature failed, r=%d", r);
        return r;
    }

    const char* sender = sd_bus_message_get_sender(m);
    assert(sender);
    TR_INFO("Starting to process RegisterResources method call from sender %s", sender);

    std::string app_resource_definition;
    std::tie(r, app_resource_definition) = get_string_argument(m, ret_error);
    if (r < 0) {
        TR_ERR("get_string_argument failed, r=%d", r);
        return r;
    }

    std::pair<CloudConnectStatus, std::string> register_resources_ret_pair =
        ccrb.register_resources(IpcConnection(sender), app_resource_definition);

    if (is_cloud_connect_error(register_resources_ret_pair.first)) {
        return handle_message_process_failure(
            register_resources_ret_pair.first, "register_resources", ret_error);
    }

    // save the token
    access_token_ = register_resources_ret_pair.second;

    // TODO - IOTMBL-1527
    // validate app registered expected interface on bus? (use sd-bus track)

    // register_resources succeeded. Send method-reply to the D-Bus connection that
    // requested register_resources invocation.
    r = reply_on_message(connection_handle, m, ret_error, register_resources_ret_pair.first);
    if (r < 0) {
        TR_ERR("reply_on_message failed, r=%d", r);
        return r;
    }

    TR_INFO("Reply on RegisterResources method successfully sent to %s", sender);
    return 0;
}

int DBusRegisterResourcesMessageProcessor::fill_reply_message(sd_bus_message* m_reply,
                                                              const std::string& member,
                                                              CloudConnectStatus status,
                                                              sd_bus_error* ret_error)
{
    TR_DEBUG_ENTER;
    UNUSED(status);
    assert(m_reply);
    assert(ret_error);
    assert(member == DBUS_CC_REGISTER_RESOURCES_METHOD_NAME);
    // access_token argument should not be null.
    assert(access_token_ != "");

    // append access_token
    int r = sd_bus_message_append(m_reply, reply_message_signature_.c_str(), access_token_.c_str());
    if (r < 0) {
        std::stringstream msg("Register resources sd_bus_message_append token " + access_token_);
        return LOG_AND_SET_SD_BUS_ERROR_F(r, ret_error, msg);
    }

    return 0;
}

/***************************   DBusDeregisterResourcesMessageProcessor ****************************/

DBusDeregisterResourcesMessageProcessor::DBusDeregisterResourcesMessageProcessor()
    : DBusCommonMessageProcessor("s", "u")
{
    TR_DEBUG_ENTER;
}

int DBusDeregisterResourcesMessageProcessor::process_message(sd_bus* connection_handle,
                                                             sd_bus_message* m,
                                                             ResourceBroker& ccrb,
                                                             sd_bus_error* ret_error)
{
    TR_DEBUG_ENTER;
    assert(connection_handle);
    assert(m);
    assert(ret_error);

    // verify the signature
    int r = verify_signature(m, ret_error);
    if (r < 0) {
        TR_ERR("verify_signature failed, r=%d", r);
        return r;
    }

    const char* sender = sd_bus_message_get_sender(m);
    assert(sender);
    TR_INFO("Starting to process DeregisterResources method call from sender %s", sender);

    std::string access_token;
    std::tie(r, access_token) = get_string_argument(m, ret_error);
    if (r < 0) {
        TR_ERR("get_string_argument failed, r=%d", r);
        return r;
    }

    // call deregister_resources resource broker API and handle output
    CloudConnectStatus cc_status = ccrb.deregister_resources(IpcConnection(sender), access_token);

    if (is_cloud_connect_error(cc_status)) {
        return handle_message_process_failure(cc_status, "deregister_resources", ret_error);
    }

    // deregister_resources succeeded. Send method-reply to the D-Bus connection that
    // requested deregister_resources invocation.
    r = reply_on_message(connection_handle, m, ret_error, cc_status);
    if (r < 0) {
        TR_ERR("reply_on_message failed, r=%d", r);
        return r;
    }

    TR_INFO("Reply on DeregisterResources method successfully sent to %s", sender);
    return 0;
}

int DBusDeregisterResourcesMessageProcessor::fill_reply_message(sd_bus_message* m_reply,
                                                                const std::string& member,
                                                                CloudConnectStatus status,
                                                                sd_bus_error* ret_error)
{
    TR_DEBUG_ENTER;
    assert(m_reply);
    assert(member == DBUS_CC_DEREGISTER_RESOURCES_METHOD_NAME);
    assert(ret_error);

    int r = sd_bus_message_append(m_reply, reply_message_signature_.c_str(), status);
    if (r < 0) {
        std::stringstream msg("Deregister resources fill in reply message: member " + member +
                              " : sd_bus_message_append status");
        return LOG_AND_SET_SD_BUS_ERROR_F(r, ret_error, msg);
    }

    return 0;
}

/******************************   DBusSetResourcesMessageProcessor  *******************************/

DBusSetResourcesMessageProcessor::DBusSetResourcesMessageProcessor()
    : DBusCommonMessageProcessor("sa(sv)" /* message format */, "" /* reply format */)
{
    TR_DEBUG_ENTER;
}

int DBusSetResourcesMessageProcessor::process_message(sd_bus* connection_handle,
                                                      sd_bus_message* m,
                                                      ResourceBroker& ccrb,
                                                      sd_bus_error* ret_error)
{
    TR_DEBUG_ENTER;
    assert(connection_handle);
    assert(m);
    assert(ret_error);

    // verify the signature
    int r = verify_signature(m, ret_error);
    if (r < 0) {
        TR_ERR("verify_signature failed, r=%d", r);
        return r;
    }

    const char* sender = sd_bus_message_get_sender(m);
    assert(sender);
    TR_INFO("Starting to process SetResourcesValues method call from sender %s", sender);

    std::string access_token;
    std::tie(r, access_token) = get_string_argument(m, ret_error);
    if (r < 0) {
        TR_ERR("get_string_argument failed, return = %d", r);
        return r;
    }

    std::vector<ResourceSetOperation> inout_set_operations;
    r = read_array_from_message(m, inout_set_operations, ret_error);
    if (r < 0) {
        TR_ERR("read_array_from_message failed, return = %d", r);
        return r;
    }

    // call set_resources_values resource broker API
    CloudConnectStatus out_status =
        ccrb.set_resources_values(IpcConnection(sender), access_token, inout_set_operations);

    // check the reply and if there is a failure return an error reply
    if (is_message_process_failure(out_status, inout_set_operations)) {
        return handle_message_process_failure(m, inout_set_operations, out_status, ret_error);
    }

    r = reply_on_message(connection_handle, m, ret_error, out_status);
    if (r < 0) {
        TR_ERR("reply_on_message failed, return = %d", r);
        return r;
    }

    TR_INFO("Reply on SetResourcesValues method successfully sent to %s", sender);
    return 0;
}

int DBusSetResourcesMessageProcessor::fill_reply_message(sd_bus_message* m_reply,
                                                         const std::string& member,
                                                         CloudConnectStatus status,
                                                         sd_bus_error* ret_error)
{
    TR_DEBUG_ENTER;
    UNUSED(m_reply);
    assert(member == DBUS_CC_SET_RESOURCES_VALUES_METHOD_NAME);
    UNUSED(ret_error);
    UNUSED(status);

    // SetResourcesValues message reply is empty
    assert(reply_message_signature_.empty());

    return 0;
}

bool DBusSetResourcesMessageProcessor::is_message_process_failure(
    CloudConnectStatus& status, std::vector<ResourceSetOperation>& set_operations)
{
    TR_DEBUG_ENTER;

    if (is_cloud_connect_error(status)) {
        return true;
    }

    for (auto operation : set_operations) {
        if (operation.output_status_ != STATUS_SUCCESS) {
            return true;
        }
    }
    return false;
}

int DBusSetResourcesMessageProcessor::handle_message_process_failure(
    sd_bus_message* m,
    std::vector<ResourceSetOperation>& set_operations,
    CloudConnectStatus cc_status,
    sd_bus_error* ret_error)
{
    TR_DEBUG_ENTER;
    assert(m);
    assert(ret_error);

    TR_INFO("Handle resource broker failure for SetResourcesValues message");

    if (is_cloud_connect_error(cc_status)) {

        // we have cloud connect related error in resource broker
        TR_ERR("Set resources values failed with cloud connect error %s",
               CloudConnectStatus_to_str(cc_status));
        // set custom error to sd_bus_error structure.
        // sd_bus_error_set_const translates DBus format error string to negative integer
        // and returns it's value(in this case -status_to_send)
        int r = sd_bus_error_set_const(
            ret_error,
            CloudConnectStatus_error_to_DBus_format_str(cc_status), // sd_bus_error.name
            CloudConnectStatus_to_readable_str(cc_status));         // sd_bus_error.message
        assert(-cc_status == r);

        return r;
    }

    // check each set operation status
    uint32_t err_count = 0;
    CloudConnectStatus ret_status = STATUS_SUCCESS;
    std::string return_err = "Set LWM2M resources failed: ";

    for (auto operation : set_operations) {
        if (operation.output_status_ != STATUS_SUCCESS) {
            if (err_count < MAX_NUMBER_OF_ERRORS_IN_REPLY) {

                std::string current_error(operation.input_data_.get_path() + " : " +
                                          CloudConnectStatus_to_str(operation.output_status_));
                // check the error message size
                if (return_err.size() + current_error.size() <=
                    SD_BUS_RETURN_ERROR_MESSAGE_MAX_LENGTH)
                {
                    // the first error will be set on sd-bus
                    if (err_count == 0) {
                        ret_status = operation.output_status_;
                    }
                    // return message
                    return_err += current_error;
                    ++err_count;
                }
            }
            else
            {
                return_err.append(" The list of failures is partial, there are more failed set "
                                  "LWM2M resources values operations.");
                break;
            }
        }
    }

    assert(err_count > 0);
    TR_ERR("%s", return_err.c_str());

    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_object_cleaner<sd_bus_error> error_cleaner(&error, sd_bus_error_free);
    // set an error
    int r = sd_bus_error_set(
        &error, CloudConnectStatus_error_to_DBus_format_str(ret_status), return_err.c_str());
    assert(-ret_status == r);
    // return an error message
    return sd_bus_reply_method_error(m, &error);
}

int DBusSetResourcesMessageProcessor::read_array_from_message(
    sd_bus_message* m,
    std::vector<ResourceSetOperation>& resource_set_operation,
    sd_bus_error* ret_error)
{
    TR_DEBUG_ENTER;
    assert(m);
    assert(ret_error);

    std::string member = sd_bus_message_get_member(m);
    std::string sender = sd_bus_message_get_sender(m);

    // enter array
    int r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "(sv)");
    if (r < 0) {
        std::stringstream msg("Set resources values: member " + member + ", sender " + sender +
                              " : sd_bus_message_enter_container array error");
        return LOG_AND_SET_SD_BUS_ERROR_F(EBADMSG, ret_error, msg);
    }
    // enter struct
    while ((r = sd_bus_message_enter_container(m, SD_BUS_TYPE_STRUCT, "sv")) > 0) {
        // read path, empty path is checked inside get_string_argument()
        std::string resource_path;
        std::tie(r, resource_path) = get_string_argument(m, ret_error);
        if (r < 0) {
            TR_ERR("get_string_argument resource path failed, return = %d", r);
            return r;
        }

        // read variant type
        const char* contents = nullptr;
        r = sd_bus_message_peek_type(m, nullptr, &contents);
        if (r < 0) {
            std::stringstream msg("Set resources values: member " + member + ", sender " + sender +
                                  " : sd_bus_message_peek_type error");
            return LOG_AND_SET_SD_BUS_ERROR_F(EFAULT, ret_error, msg);
        }
        // enter variant
        r = sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT, contents);
        if (r < 0) {
            std::stringstream msg("Set resources values: member " + member + ", sender " + sender +
                                  " : sd_bus_message_enter_container variant error");
            return LOG_AND_SET_SD_BUS_ERROR_F(EFAULT, ret_error, msg);
        }
        char variant_inner_type = _SD_BUS_TYPE_INVALID;
        r = sd_bus_message_peek_type(m, &variant_inner_type, &contents);
        if (r < 0) {
            std::stringstream msg("Set resources values: member " + member + ", sender " + sender +
                                  " : sd_bus_message_peek_type error");
            return LOG_AND_SET_SD_BUS_ERROR_F(EFAULT, ret_error, msg);
        }

        switch (variant_inner_type)
        {
        case SD_BUS_TYPE_STRING:
        {
            std::string s_value;
            std::tie(r, s_value) = get_string_argument(m, ret_error);
            if (r < 0) {
                TR_ERR("get_string_argument failed, return = %d", r);
                return r;
            }

            resource_set_operation.push_back(ResourceData(resource_path, s_value));

            break;
        }
        case SD_BUS_TYPE_INT64:
        {
            int64_t int64_value = 0;
            r = sd_bus_message_read_basic(m, SD_BUS_TYPE_INT64, &int64_value);
            if (r < 0) {
                std::stringstream msg("Set resources values: member " + member + ", sender " +
                                      sender +
                                      " : sd_bus_message_read_basic SD_BUS_TYPE_INT64 error");
                return LOG_AND_SET_SD_BUS_ERROR_F(EBADMSG, ret_error, msg);
            }

            ResourceData data(resource_path, int64_value);
            resource_set_operation.push_back(std::move(data));

            break;
        }
        default:
        {
            std::stringstream msg("Set resources values: member " + member + ", sender " + sender +
                                  ": unsupported variant type " +
                                  std::string(stringify(data_type)));
            return LOG_AND_SET_SD_BUS_ERROR_F(EFAULT, ret_error, msg);
        }
        }

        r = sd_bus_message_exit_container(m);
        if (r < 0) {
            std::stringstream msg("Set resources values: member " + member + ", sender " + sender +
                                  " sd_bus_message_exit_container");
            return LOG_AND_SET_SD_BUS_ERROR_F(EFAULT, ret_error, msg);
        }
    }

    r = sd_bus_message_exit_container(m);

    if (r < 0) {
        std::stringstream msg("Member " + member + ", sender " + sender +
                              " : sd_bus_message_exit_container");
        return LOG_AND_SET_SD_BUS_ERROR_F(EFAULT, ret_error, msg);
    }

    return 0;
}

/******************************   DBusGetResourcesMessageProcessor  *******************************/

DBusGetResourcesMessageProcessor::DBusGetResourcesMessageProcessor()
    : DBusCommonMessageProcessor("sa(sy)", "a(yv)")
{
    TR_DEBUG_ENTER;
}

int DBusGetResourcesMessageProcessor::process_message(sd_bus* connection_handle,
                                                      sd_bus_message* m,
                                                      ResourceBroker& ccrb,
                                                      sd_bus_error* ret_error)
{
    TR_DEBUG_ENTER;
    assert(connection_handle);
    assert(m);
    assert(ret_error);

    // verify the signature
    int r = verify_signature(m, ret_error);
    if (r < 0) {
        TR_ERR("verify_signature failed, r=%d", r);
        return r;
    }

    const char* sender = sd_bus_message_get_sender(m);
    assert(sender);
    TR_INFO("Starting to process GetResourcesValues method call from sender %s", sender);

    std::string access_token;
    std::tie(r, access_token) = get_string_argument(m, ret_error);
    if (r < 0) {
        TR_ERR("get_string_argument failed, return = %d", r);
        return r;
    }

    get_operations_.clear();
    r = read_array_from_message(m, get_operations_, ret_error);
    if (r < 0) {
        TR_ERR("read_array_from_message failed, return = %d", r);
        return r;
    }

    // call deregister_resources resource broker API and handle output
    CloudConnectStatus out_status =
        ccrb.get_resources_values(IpcConnection(sender), access_token, get_operations_);

    if (is_message_process_failure(out_status, get_operations_)) {
        return handle_message_process_failure(m, get_operations_, out_status, ret_error);
    }

    // deregister_resources succeeded. Send method-reply to the D-Bus connection that
    // requested deregister_resources invocation.
    r = reply_on_message(connection_handle, m, ret_error, out_status);
    if (r < 0) {
        TR_ERR("reply_on_message failed, return = %d", r);
        return r;
    }

    return 0;
}

int DBusGetResourcesMessageProcessor::fill_reply_message(sd_bus_message* m_reply,
                                                         const std::string& member,
                                                         CloudConnectStatus status,
                                                         sd_bus_error* ret_error)
{
    TR_DEBUG_ENTER;
    UNUSED(status);
    assert(m_reply);
    assert(ret_error);
    assert(member == DBUS_CC_GET_RESOURCES_VALUES_METHOD_NAME);

    int r = 0;

    // TODO: implement multiple get operation
    if (get_operations_.size() != 1) {
        std::stringstream msg(
            "Fail: get LWM2M resource values support for a single resource only!");
        return LOG_AND_SET_SD_BUS_ERROR_F(EFAULT, ret_error, msg);
    }

    for (auto& operations : get_operations_) {

        ResourceDataType data_type = operations.inout_data_.get_data_type();
        switch (data_type)
        {
        case STRING:
        {
            r = sd_bus_message_append(m_reply,
                                      reply_message_signature_.c_str(),
                                      1,
                                      static_cast<uint8_t>(data_type),
                                      "s",
                                      operations.inout_data_.get_value_string().c_str());
            break;
        }
        case INTEGER:
        {
            r = sd_bus_message_append(m_reply,
                                      reply_message_signature_.c_str(),
                                      1,
                                      static_cast<uint8_t>(data_type),
                                      "x",
                                      operations.inout_data_.get_value_integer());
            break;
        }

        default:

        {
            std::stringstream msg("Get resources values, fill in message: data type " +
                                  std::string(stringify(data_type)) + " is not supported, member " +
                                  member);
            return LOG_AND_SET_SD_BUS_ERROR_F(EFAULT, ret_error, msg);
        }
        }

        if (r < 0) {
            std::stringstream msg("Get resources values, fill in message: data type "
                                  "sd_bus_message_append failed! Message reply format: " +
                                  reply_message_signature_ + ", member " + member);
            return LOG_AND_SET_SD_BUS_ERROR_F(r, ret_error, msg);
        }
    }

    return 0;
}

bool DBusGetResourcesMessageProcessor::is_message_process_failure(
    CloudConnectStatus& status, std::vector<ResourceGetOperation>& get_operations)
{
    TR_DEBUG_ENTER;

    if (is_cloud_connect_error(status)) {
        return true;
    }

    for (auto operation : get_operations) {
        if (operation.output_status_ != STATUS_SUCCESS) {
            return true;
        }
    }
    return false;
}

int DBusGetResourcesMessageProcessor::handle_message_process_failure(
    sd_bus_message* m,
    std::vector<ResourceGetOperation>& get_operations,
    CloudConnectStatus cc_status,
    sd_bus_error* ret_error)
{
    TR_DEBUG_ENTER;
    assert(m);
    assert(ret_error);

    if (is_cloud_connect_error(cc_status)) {

        // we have cloud connect related error in resource broker
        TR_ERR("Get resources values failed with cloud connect error %s",
               CloudConnectStatus_to_str(cc_status));

        // set custom error to sd_bus_error structure.
        // sd_bus_error_set_const translates DBus format error string to negative integer
        // and returns it's value(in this case -status_to_send)
        int r = sd_bus_error_set_const(
            ret_error,
            CloudConnectStatus_error_to_DBus_format_str(cc_status), // sd_bus_error.name
            CloudConnectStatus_to_readable_str(cc_status));         // sd_bus_error.message
        assert(-cc_status == r);
        return r;
    }

    // check each set operation
    uint32_t err_count = 0;
    CloudConnectStatus cc_err_status = STATUS_SUCCESS;
    std::string return_err = "Get LWM2M resources failed: ";

    for (auto operation : get_operations) {
        if (operation.output_status_ != STATUS_SUCCESS) {

            if (err_count < MAX_NUMBER_OF_ERRORS_IN_REPLY) {

                std::string current_error(operation.inout_data_.get_path() + " : " +
                                          CloudConnectStatus_to_str(operation.output_status_));
                // check the error message size
                if (return_err.size() + current_error.size() <=
                    SD_BUS_RETURN_ERROR_MESSAGE_MAX_LENGTH)
                {
                    // the first error will be set on sd-bus
                    if (err_count == 0) {
                        cc_err_status = operation.output_status_;
                    }
                    // append to message
                    return_err += current_error;
                    ++err_count;
                }
            }
            else
            {
                return_err.append(" The list of failures is partial, there are more failed get"
                                  "LWM2M resources values operations.");
                break;
            }
        }
    }

    assert(err_count > 0);
    TR_ERR("%s", return_err.c_str());

    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_object_cleaner<sd_bus_error> error_cleaner(&error, sd_bus_error_free);
    // set an error
    int r = sd_bus_error_set(
        &error, CloudConnectStatus_error_to_DBus_format_str(cc_err_status), return_err.c_str());
    assert(-cc_err_status == r);
    // send an error reply
    return sd_bus_reply_method_error(m, &error);
}

int DBusGetResourcesMessageProcessor::read_array_from_message(
    sd_bus_message* m,
    vector<ResourceGetOperation>& resource_get_operation,
    sd_bus_error* ret_error)
{
    TR_DEBUG_ENTER;

    assert(m);
    assert(resource_get_operation.empty());
    assert(ret_error);

    std::string member = sd_bus_message_get_member(m);
    std::string sender = sd_bus_message_get_sender(m);

    // enter an array container
    int r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "(sy)");
    if (r < 0) {
        std::stringstream msg("Get resources values, member " + member + ", sender " + sender +
                              " : sd_bus_message_enter_container array");
        return LOG_AND_SET_SD_BUS_ERROR_F(EBADMSG, ret_error, msg);
    }
    // enter struct container
    while ((r = sd_bus_message_enter_container(m, SD_BUS_TYPE_STRUCT, "sy")) > 0) {
        // read path
        std::string resource_path;
        std::tie(r, resource_path) = get_string_argument(m, ret_error);
        if (r < 0) {
            TR_ERR("get_string_argument failed, return = %d", r);
            return r;
        }

        uint8_t u_value = 0;
        r = sd_bus_message_read_basic(m, SD_BUS_TYPE_BYTE, &u_value);
        if (r < 0) {
            std::stringstream msg("Get resources values, member " + member + ", sender " + sender +
                                  " : sd_bus_message_read_basic SD_BUS_TYPE_BYTE");
            return LOG_AND_SET_SD_BUS_ERROR_F(EBADMSG, ret_error, msg);
        }

        ResourceGetOperation get_operation(resource_path, (ResourceDataType) u_value);
        resource_get_operation.push_back(std::move(get_operation));

        // exit struct container
        r = sd_bus_message_exit_container(m);
        if (r < 0) {
            std::stringstream msg("Get resources values, member " + member + ", sender " + sender +
                                  " : exit struct container sd_bus_message_exit_container error");
            return LOG_AND_SET_SD_BUS_ERROR_F(r, ret_error, msg);
        }
    }

    // exit an array container
    r = sd_bus_message_exit_container(m);
    if (r < 0) {
        std::stringstream msg("Get resources values, member " + member + ", sender " + sender +
                              " : exit an array container sd_bus_message_exit_container error");
        return LOG_AND_SET_SD_BUS_ERROR_F(r, ret_error, msg);
    }

    return 0;
}

} // namespace mbl {
