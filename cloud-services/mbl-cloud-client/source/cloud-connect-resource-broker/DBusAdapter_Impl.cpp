/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "DBusAdapter.h"
#include "CloudConnectTrace.h"
#include "CloudConnectTypes.h"
#include "DBusAdapter_Impl.h"
#include "DBusService.h"
#include "Mailbox.h"
#include "MailboxMsg.h"
#include "ResourceBroker.h"

#include <cassert>
#include <stdbool.h>
#include <string>
#include <vector>

#define TRACE_GROUP "ccrb-dbus"

namespace mbl
{

DBusAdapterImpl::DBusAdapterImpl(ResourceBroker& ccrb)
    : ccrb_(ccrb),
      connection_handle_(nullptr),
      unique_name_(nullptr),
      service_name_(nullptr),
      event_loop_handle_(nullptr),
      mailbox_in_("incoming messages mailbox"),
      initializer_thread_id_(0)
{
    TR_DEBUG("Enter");
    state_.set(DBusAdapterState::UNINITALIZED);
}

// TODO - IOTMBL-1606 - decide what matches we add and which we remove - more research should be
// done
const std::vector<std::pair<std::string, std::string>> DBusAdapterImpl::match_rules{
    std::make_pair("NameOwnerChanged",
                   "type='signal',interface='org.freedesktop.DBus',member='NameOwnerChanged'"),
    std::make_pair("NameLost", "type='signal',interface='org.freedesktop.DBus',member='NameLost'"),
    std::make_pair("NameAcquired",
                   "type='signal',interface='org.freedesktop.DBus',member='NameAcquired'")};

MblError DBusAdapterImpl::add_match_rules()
{
    for (const auto& elem : DBusAdapterImpl::match_rules) {
        int r = sd_bus_add_match(connection_handle_,
                                 nullptr, // floating source , I see for now no reason to get a slot
                                 elem.second.c_str(),
                                 DBusAdapterImpl::incoming_bus_message_callback,
                                 this);
        if (r < 0) {
            TR_ERR("sd_bus_add_match failed adding rull [%s] , with error r=%d (%s) - returning %s",
                   elem.second.c_str(),
                   r,
                   strerror(r),
                   MblError_to_str(MblError::DBA_SdBusRequestAddMatchFailed));
            return MblError::DBA_SdBusRequestAddMatchFailed;
        }
        TR_INFO("Added D-Bus broker match rule - %s", elem.first.c_str());
    }
    return MblError::None;
}

// Cloud Connect errors to D-Bus format error strings translation map.
// Information from this map is used when D-Bus ifrastructure translates
// sd_bus_error.name field string to the negative integer value.
const sd_bus_error_map cloud_connect_dbus_errors[] = {
    SD_BUS_ERROR_MAP(CLOUD_CONNECT_ERR_FAILED, ERR_FAILED),
    SD_BUS_ERROR_MAP(CLOUD_CONNECT_ERR_INTERNAL_ERROR, ERR_INTERNAL_ERROR),
    SD_BUS_ERROR_MAP(CLOUD_CONNECT_ERR_INVALID_APPLICATION_RESOURCES_DEFINITION,
                     ERR_INVALID_APPLICATION_RESOURCES_DEFINITION),
    SD_BUS_ERROR_MAP(CLOUD_CONNECT_ERR_REGISTRATION_ALREADY_IN_PROGRESS,
                     ERR_REGISTRATION_ALREADY_IN_PROGRESS),
    SD_BUS_ERROR_MAP(CLOUD_CONNECT_ERR_ALREADY_REGISTERED, ERR_ALREADY_REGISTERED),

    SD_BUS_ERROR_MAP_END};

MblError DBusAdapterImpl::bus_init()
{
    TR_DEBUG("Enter");

    // Enforce initialization of event loop before bus
    if (nullptr == event_loop_handle_) {
        TR_ERR("event_loop_handle_ not initialized! returning %s",
               MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }

    // Open a connection to the bus. DBUS_SESSION_BUS_ADDRESS should be define as part of the
    // process environment
    int r = sd_bus_open_user(&connection_handle_);
    if (r < 0) {
        TR_ERR("sd_bus_open_user failed with error r=%d (%s) - returning %s",
               r,
               strerror(r),
               MblError_to_str(MblError::DBA_SdBusCallFailure));
        return MblError::DBA_SdBusCallFailure;
    }
    assert(connection_handle_);
    // FIXME - remove later (keep for debug)
    // TR_DEBUG("D-Bus Connection object created (connection_handle_=%p)", connection_handle_);

    // Attach bus connection object to event loop
    r = sd_bus_attach_event(connection_handle_, event_loop_handle_, SD_EVENT_PRIORITY_NORMAL);
    if (r < 0) {
        TR_ERR("sd_bus_attach_event failed with error r=%d (%s) - returning %s",
               r,
               strerror(r),
               MblError_to_str(MblError::DBA_SdBusCallFailure));
        return MblError::DBA_SdBusCallFailure;
    }
    TR_INFO("Connection object attached to event object");

    // Attach sd-bus vtable Interface DBUS_CLOUD_CONNECT_INTERFACE_NAME under object path
    // DBUS_CLOUD_CONNECT_OBJECT_PATH to the bus connection
    // The vtable is our publish service
    // userdata for all callbacks - 'this'
    const sd_bus_vtable* service_vtable = DBusService_get_service_vtable();
    assert(service_vtable);
    r = sd_bus_add_object_vtable(connection_handle_,
                                 nullptr,
                                 DBUS_CLOUD_CONNECT_OBJECT_PATH,
                                 DBUS_CLOUD_CONNECT_INTERFACE_NAME,
                                 service_vtable,
                                 this);
    if (r < 0) {
        TR_ERR("sd_bus_add_object_vtable failed with error r=%d (%s) - returning %s",
               r,
               strerror(r),
               MblError_to_str(MblError::DBA_SdBusCallFailure));
        return MblError::DBA_SdBusCallFailure;
    }
    TR_INFO("Added new interface %s using service_vtable to object %s",
            DBUS_CLOUD_CONNECT_INTERFACE_NAME,
            DBUS_CLOUD_CONNECT_OBJECT_PATH);

    // Get my unique name on the bus
    r = sd_bus_get_unique_name(connection_handle_, &unique_name_);
    if (r < 0) {
        TR_ERR("sd_bus_get_unique_name failed with error r=%d (%s) - returning %s",
               r,
               strerror(r),
               MblError_to_str(MblError::DBA_SdBusCallFailure));
        return MblError::DBA_SdBusCallFailure;
    }
    assert(unique_name_);
    TR_INFO("unique_name_=%s", unique_name_);

    // Request a well-known service name DBUS_CLOUD_SERVICE_NAME so client Apps can find us
    // We do not expect anyone else to already own that name
    r = sd_bus_request_name(connection_handle_, DBUS_CLOUD_SERVICE_NAME, 0);
    if (r < 0) {
        TR_ERR("sd_bus_request_name failed with error r=%d (%s) - returning %s",
               r,
               strerror(r),
               MblError_to_str(MblError::DBA_SdBusRequestNameFailed));
        return MblError::DBA_SdBusRequestNameFailed;
    }
    service_name_ = DBUS_CLOUD_SERVICE_NAME;
    TR_INFO("Aquired D-Bus known name service_name_=%s", DBUS_CLOUD_SERVICE_NAME);

    // add Cloud Connect D-Bus errors mapping
    r = sd_bus_error_add_map(cloud_connect_dbus_errors);
    if (r < 0) {
        TR_ERR("sd_bus_error_add_map failed with error r=%d (%s) - returning %s",
               r,
               strerror(r),
               MblError_to_str(MblError::DBA_SdBusCallFailure));
        return MblError::DBA_SdBusCallFailure;
    }

    // TODO - IOTMBL-1606 - Add match rules -this part is commented since messages are not yet
    // handled on callback
    // MblError status = add_match_rules();
    // if (status != MblError::None){
    //    TR_ERR("set_match_rules() failed with error %s", MblError_to_str(status));
    //    return status;
    //}

    return MblError::None;
}

MblError DBusAdapterImpl::bus_deinit()
{
    TR_DEBUG("Enter");

    // sd_bus_flush_close_unref return always nullptr
    connection_handle_ = sd_bus_flush_close_unref(connection_handle_);
    service_name_ = nullptr;
    unique_name_ = nullptr;

    return MblError::None;
}

MblError DBusAdapterImpl::event_loop_init()
{
    TR_DEBUG("Enter");

    // Create the sd-event loop object (thread loop)
    int r = sd_event_default(&event_loop_handle_);
    if (r < 0) {
        TR_ERR("sd_event_default failed with error r=%d (%s) - returning %s",
               r,
               strerror(r),
               MblError_to_str(MblError::DBA_SdEventCallFailure));
        return MblError::DBA_SdEventCallFailure;
    }
    assert(event_loop_handle_);
    // FIXME - remove later (keep for debug)
    // TR_DEBUG("Acquired an event loop object! (event_loop_handle_=%p)", event_loop_handle_);

    // Attach input side of mailbox into the event loop as an input event source
    // This other side of the mailbox get output from other threads who whish to communicate with
    // CCRB thread
    // Event source will be destroyed with the event loop ("floating")
    // Wait for event flag EPOLLIN (The associated file is available for read(2) operations)
    // The callback to invoke when the event is fired is incoming_mailbox_message_callback()
    r = sd_event_add_io(event_loop_handle_,
                        nullptr,
                        mailbox_in_.get_pipefd_read(),
                        EPOLLIN,
                        DBusAdapterImpl::incoming_mailbox_message_callback,
                        this);
    if (r < 0) {
        TR_ERR("sd_event_add_io failed with error r=%d (%s) - returning %s",
               r,
               strerror(r),
               MblError_to_str(MblError::DBA_SdEventCallFailure));
        return MblError::DBA_SdEventCallFailure;
    }
    TR_INFO("Added floating IO (input) event source to attach output from mailbox)");

    return MblError::None;
}

MblError DBusAdapterImpl::event_loop_deinit()
{
    TR_DEBUG("Enter");

    if (nullptr != event_loop_handle_) {
        sd_event_unref(event_loop_handle_);
        event_loop_handle_ = nullptr;
    }
    else
    {
        TR_WARN("event_loop_deinit called when event_loop_handle_ is nullptr!");
    }

    return MblError::None;
}

int DBusAdapterImpl::event_loop_request_stop(MblError stop_status)
{
    TR_DEBUG("Enter");

    // Must be first! only CCRB initializer thread should call this function.
    assert(pthread_equal(pthread_self(), initializer_thread_id_) != 0);

    // Send myself an exit request
    int r = sd_event_exit(event_loop_handle_, (int) stop_status);
    if (r < 0) {
        TR_ERR("sd_event_exit failed with error r=%d (%s)", r, strerror(r));
    }
    else
    {
        TR_INFO("sd_event_exit called with stop_status=%d", (int) stop_status);
    }
    return r;
}

int print_log_set_sd_bus_error_f(
    int err_num, sd_bus_error* ret_error, const char* func, int line, const char* format, ...)
{

    if (0 < sd_bus_error_is_set(ret_error)) {
        // avoid overwrite of the error that was already set
        return sd_bus_error_get_errno(ret_error);
    }

    va_list ap;
    va_start(ap, format);
    char buffer[MAX_LOG_LINE];
    vsnprintf(buffer, MAX_LOG_LINE, format, ap);
    printf("func %s, line %d %s", func, line, buffer);
    int r = sd_bus_error_set_errnofv(ret_error, err_num, format, ap);
    va_end(ap);
    return r;
}

int print_log_set_sd_bus_error(
    int err_num, sd_bus_error* ret_error, const char* func, int line, const char* method_name)
{

    if (0 < sd_bus_error_is_set(ret_error)) {
        // avoid overwrite of the error that was already set
        return sd_bus_error_get_errno(ret_error);
    }

    printf("func %s, line %d %s failed errno = %s (%d)",
           func,
           line,
           method_name,
           strerror(err_num),
           err_num);
    return sd_bus_error_set_errno(ret_error, err_num);
}

int DBusAdapterImpl::incoming_mailbox_message_callback_impl(sd_event_source* s,
                                                            int fd,
                                                            uint32_t revents)
{
    assert(s);
    TR_DEBUG("Enter");
    UNUSED(s);

    // Validate that revents contains epoll read event flag
    if ((revents & EPOLLIN) == 0) {
        // TODO : not sure if this error is possible - if it is -
        // we need to restart thread/process or target (??)

        TR_ERR("(revents & EPOLLIN == 0), returning -EBADFD to  disable event source");
        return (-EBADFD);
    }

    // Another validation - given fd is the one belongs to the mailbox (input side)
    if (fd != mailbox_in_.get_pipefd_read()) {
        // TODO : handle on upper layer - need to notify somehow
        TR_ERR("fd does not belong to incoming mailbox_in_,"
               "returning %d to disable event source",
               -EBADF);
        return (-EBADF);
    }

    // Read the incoming message pointer. BLock for up to 1sec (we should block at all in practice)
    auto ret_pair = mailbox_in_.receive_msg();
    if (ret_pair.first != MblError::None) {
        TR_ERR("mailbox_in_.receive_msg failed s, disable event source! returning error -EINVAL");
        return (-EINVAL);
    }

    // Process message
    MailboxMsg& msg = ret_pair.second;
    switch (msg.get_type())
    {
    case mbl::MailboxMsg::MsgType::EXIT:
    {
        // EXIT message

        // validate length (sanity check)
        if (msg.get_payload_len() != sizeof(mbl::MailboxMsg::MsgPayload::MsgExit)) {
            TR_ERR("Unexpected EXIT message length %zu (expected %zu), returning error=%s",
                   msg.get_payload_len(),
                   sizeof(mbl::MailboxMsg::MsgType::EXIT),
                   MblError_to_str(MblError::DBA_MailBoxInvalidMsg));
            return (-EBADMSG);
        }

        // External thread request to stop event loop
        auto& payload = msg.get_payload();
        TR_INFO("receive message EXIT : sending stop request to event loop with stop status=%s",
                MblError_to_str(payload.exit_.stop_status));
        int r = event_loop_request_stop(payload.exit_.stop_status);
        if (r < 0) {
            TR_ERR("event_loop_request_stop() failed with error %s (r=%d)", strerror(r), r);
            return r;
        }
        break;
    }
    case mbl::MailboxMsg::MsgType::RAW_DATA:
        // (used for testing) - ignore
        break;

    default:
        // This should never happen
        TR_ERR("Unexpected MsgType.. Ignoring..");
        break;
    }

    return 0; // success
}

int DBusAdapterImpl::incoming_mailbox_message_callback(sd_event_source* s,
                                                       int fd,
                                                       uint32_t revents,
                                                       void* userdata)
{
    assert(s);
    assert(userdata);
    TR_DEBUG("Enter");
    DBusAdapterImpl* adapter_impl = static_cast<DBusAdapterImpl*>(userdata);
    return adapter_impl->incoming_mailbox_message_callback_impl(s, fd, revents);
}

int DBusAdapterImpl::incoming_bus_message_callback(sd_bus_message* m,
                                                   void* userdata,
                                                   sd_bus_error* ret_error)
{
    TR_DEBUG("Enter");
    assert(userdata);
    assert(m);
    assert(ret_error);

    // TODO - For all failues here, might need to send an error reply ONLY if the message is of
    // kind method_call (can check that) check what is done in other implementations
    // see https://www.freedesktop.org/software/systemd/man/sd_bus_message_get_type.html#

    // TODO-  IOTMBL-1606 - add handling of matching rules. they can come from any interface
    // (for now only standard)
    if (sd_bus_message_is_empty(m) >= 0) {
        return PRINT_LOG_SET_SD_BUS_ERROR_F(EBADMSG, ret_error, "Received an empty message!");
    }

    // Expect message with our known name, directly sent to us (unicast)
    if (0 != strncmp(sd_bus_message_get_destination(m),
                     DBUS_CLOUD_SERVICE_NAME,
                     strlen(DBUS_CLOUD_SERVICE_NAME)))
    {
        return PRINT_LOG_SET_SD_BUS_ERROR_F(EFAULT,
                                            ret_error,
                                            "Received message to wrong destination (%s)!",
                                            sd_bus_message_get_destination(m));
    }

    // Expect message to a single object patch DBUS_CLOUD_CONNECT_OBJECT_PATH
    if (0 != strncmp(sd_bus_message_get_path(m),
                     DBUS_CLOUD_CONNECT_OBJECT_PATH,
                     strlen(DBUS_CLOUD_CONNECT_OBJECT_PATH)))
    {
        return PRINT_LOG_SET_SD_BUS_ERROR_F(
            EFAULT, ret_error, "Unexisting object path (%s)!", sd_bus_message_get_path(m));
    }

    // Expect message to a single interface DBUS_CLOUD_CONNECT_INTERFACE_NAME
    if (0 != strncmp(sd_bus_message_get_interface(m),
                     DBUS_CLOUD_CONNECT_INTERFACE_NAME,
                     strlen(DBUS_CLOUD_CONNECT_INTERFACE_NAME)))
    {
        return PRINT_LOG_SET_SD_BUS_ERROR_F(
            EFAULT, ret_error, "Unexisting interface (%s)!", sd_bus_message_get_interface(m));
    }

    /*
    At this stage we are sure to handle all messages to known service DBUS_CLOUD_SERVICE_NAME,
    Object DBUS_CLOUD_CONNECT_OBJECT_PATH and interface DBUS_CLOUD_CONNECT_INTERFACE_NAME
    Messages can be of type Signal / Error / Method Call.
    */
    uint8_t type = 0;
    int r = sd_bus_message_get_type(m, &type);
    if (r < 0) {
        return PRINT_LOG_SET_SD_BUS_ERROR(r, ret_error, "sd_bus_message_get_type");
    }
    if (!(is_valid_message_type(type))) {
        return PRINT_LOG_SET_SD_BUS_ERROR_F(ENOMSG,
                                            ret_error,
                                            "Received message from a wrong type (%s)!",
                                            message_type_to_str(type));
    }

    TR_INFO("Received message of type %s from sender %s",
            message_type_to_str(type),
            sd_bus_message_get_sender(m));

    r = -EBADRQC;
    const char* member_name = nullptr;
    DBusAdapterImpl* impl = static_cast<DBusAdapterImpl*>(userdata);
    if (0 <= sd_bus_message_is_method_call(m, nullptr, DBUS_CC_REGISTER_RESOURCES_METHOD_NAME)) {
        member_name = DBUS_CC_REGISTER_RESOURCES_METHOD_NAME;
        r = impl->process_message_RegisterResources(m, ret_error);
        if (r < 0) {
            TR_ERR("process_message_RegisterResources failed!");
        }
    }
    else if (0 <=
             sd_bus_message_is_method_call(m, nullptr, DBUS_CC_DEREGISTER_RESOURCES_METHOD_NAME))
    {
        member_name = DBUS_CC_DEREGISTER_RESOURCES_METHOD_NAME;
        r = impl->process_message_DeregisterResources(m, ret_error);
        if (r < 0) {
            TR_ERR("process_message_DeregisterResources failed!");
        }
    }
    else
    {
        // TODO - probably need to reply with error reply to sender?
        TR_ERR("Received a message with unknown member=%s!", sd_bus_message_get_member(m));
        assert(0);
    }

    TR_DEBUG("Message of type %s member name %s from sender %s status:",
             message_type_to_str(type),
             member_name,
             sd_bus_message_get_sender(m));

    // handle and print to logs handling status
    if (r == 0) {
        TR_DEBUG("Processed with success");
    }
    else
    {
        if (r > 0) {
            // positive value of r is unexpected
            TR_DEBUG("Processed with unexpected status r=%d (greater that 0)!"
                     " Returning as negative error (%d)",
                     r,
                     -r);
            r = (-r); // return negative status
        }
        else
        {
            // r is negative, that means failure
            TR_DEBUG("Processed with failure status r=%d!", r);
        }
    }

    return r;
}

int DBusAdapterImpl::handle_resource_broker_async_method_success(sd_bus_message* m_to_reply_on,
                                                                 sd_bus_error* ret_error,
                                                                 const char* types_format,
                                                                 CloudConnectStatus status,
                                                                 const char* access_token)
{
    TR_DEBUG("Enter");

    assert(m_to_reply_on);
    assert(types_format);

    // An asynchronous process towards the cloud was finished successfully,
    // so we need to store handle.
    if (!(pending_messages_.insert(m_to_reply_on).second)) {
        TR_ERR("pending_messages_.insert failed!");
        return sd_bus_error_set_const(ret_error,
                                      CloudConnectStatus_error_to_DBus_str(ERR_INTERNAL_ERROR),
                                      CloudConnectStatus_to_str(ERR_INTERNAL_ERROR));
    }

    int r = method_reply_on_message(m_to_reply_on, ret_error, types_format, status, access_token);
    if (r < 0) {
        // sending reply failed. remove stored message.
        pending_messages_.erase(m_to_reply_on);
        TR_ERR("method_reply_on_message failed!");
        return r;
    }

    // Increase refcount of the message in order to avoid message deallocation
    // until async process will be finished.
    sd_bus_message_ref(m_to_reply_on);

    return 0;
}

int DBusAdapterImpl::method_reply_on_message(sd_bus_message* m_to_reply_on,
                                             sd_bus_error* ret_error,
                                             const char* types_format,
                                             CloudConnectStatus status,
                                             const char* access_token)
{
    TR_DEBUG("Enter");

    assert(m_to_reply_on);
    assert(types_format);

    const char* method_name = sd_bus_message_get_member(m_to_reply_on);
    const char* sender_name = sd_bus_message_get_sender(m_to_reply_on);
    TR_DEBUG("Sending reply according to types_format=%s on %s method to %s",
             types_format,
             method_name,
             sender_name);

    sd_bus_message* reply = nullptr;

    // When we leave this function scope, we need to call sd_bus_message_unrefp on reply
    sd_objects_cleaner<sd_bus_message> reply_cleaner(reply, sd_bus_message_unref);

    int r = sd_bus_message_new_method_return(m_to_reply_on, &reply);
    if (r < 0) {
        return PRINT_LOG_SET_SD_BUS_ERROR(r, ret_error, "sd_bus_message_new_method_return");
    }

    if (0 == strcmp(types_format, "us")) {
        // we expect types_format = "us" for RegisterResources.
        // access_token argument should not be null.
        assert(access_token);
        r = sd_bus_message_append(reply, types_format, status, access_token);
    }
    else if (0 == strcmp(types_format, "u"))
    {
        // we expect types_format = "u" for DeregisterResources, AddResourceInstances
        // and RemoveResourceInstances.
        r = sd_bus_message_append(reply, types_format, status);
    }
    else
    {
        TR_ERR("Unexpected types_format (%s) in reply on %s method to %s",
               types_format,
               method_name,
               sender_name);
        assert(0);
    }

    if (r < 0) {
        return PRINT_LOG_SET_SD_BUS_ERROR(r, ret_error, "sd_bus_message_append");
    }

    r = sd_bus_send(connection_handle_, reply, nullptr);
    if (r < 0) {
        return PRINT_LOG_SET_SD_BUS_ERROR(r, ret_error, "sd_bus_send");
    }

    TR_DEBUG("Reply on %s method successfully sent to %s", method_name, sender_name);

    return 0;
}

int DBusAdapterImpl::handle_resource_broker_method_failure(MblError mbl_status,
                                                           CloudConnectStatus cc_status,
                                                           const char* method_name,
                                                           sd_bus_error* ret_error)
{
    TR_DEBUG("Enter");

    assert(MblError::None != mbl_status || is_CloudConnectStatus_error(cc_status));

    CloudConnectStatus status_to_send = ERR_INTERNAL_ERROR;
    if (MblError::None != mbl_status) {
        // we have internal error in resource broker
        TR_ERR("%s failed with MblError %s", method_name, MblError_to_str(mbl_status));
    }
    else
    {
        // we have cloud connect related error in resource broker
        TR_ERR("%s failed with cloud connect error %s",
               method_name,
               CloudConnectStatus_to_str(cc_status));
        status_to_send = cc_status;
    }

    // set custom error to sd_bus_error structure.
    // sd_bus_error_set_const translates DBus format error string to negative integer
    // and returns it's value(in this case -status_to_send)
    return sd_bus_error_set_const(
        ret_error,
        CloudConnectStatus_error_to_DBus_str(status_to_send), // sd_bus_error.name
        CloudConnectStatus_to_readable_str(status_to_send));  // sd_bus_error.message
}

int DBusAdapterImpl::verify_signature_and_get_string_argument(sd_bus_message* m,
                                                              sd_bus_error* ret_error,
                                                              std::string& out_string)
{
    TR_DEBUG("Enter");
    assert(m);

    if (sd_bus_message_get_expect_reply(m) == 0) {
        // reply to the message m is not expected.
        return PRINT_LOG_SET_SD_BUS_ERROR_F(
            ENOMSG, ret_error, "Unexpected message type: no reply expected!");
    }

    if (!(sd_bus_message_has_signature(m, "s"))) {
        return PRINT_LOG_SET_SD_BUS_ERROR_F(ENOMSG, ret_error, "Unexpected message signature!");
    }

    const char* out_read = nullptr;
    int r = sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &out_read);
    if (r < 0) {
        return PRINT_LOG_SET_SD_BUS_ERROR(r, ret_error, "sd_bus_message_read_basic");
    }

    if ((nullptr == out_read) || (strlen(out_read) == 0)) {
        return PRINT_LOG_SET_SD_BUS_ERROR_F(
            EINVAL, ret_error, "Invalid message argument: empty string!");
    }

    out_string = std::string(out_read);

    return 0;
}

int DBusAdapterImpl::process_message_RegisterResources(sd_bus_message* m, sd_bus_error* ret_error)
{
    TR_DEBUG("Enter");
    assert(m);

    TR_INFO("Starting to process RegisterResources method call from sender %s",
            sd_bus_message_get_sender(m));

    std::string app_resource_definition;
    int r = verify_signature_and_get_string_argument(m, ret_error, app_resource_definition);
    if (r < 0) {
        TR_ERR("verify_signature_and_get_string_argument failed!");
        return r;
    }

    // call register_resources resource broker APi and handle output
    CloudConnectStatus out_cc_reg_status = ERR_FAILED;
    std::string out_access_token;
    MblError mbl_reg_err = ccrb_.register_resources(reinterpret_cast<uintptr_t>(m),
                                                    app_resource_definition,
                                                    out_cc_reg_status,
                                                    out_access_token);

    if (MblError::None != mbl_reg_err || is_CloudConnectStatus_error(out_cc_reg_status)) {
        return handle_resource_broker_method_failure(
            mbl_reg_err, out_cc_reg_status, "register_resources", ret_error);
    }

    // TODO - IOTMBL-1527
    // validate app registered expected interface on bus? (use sd-bus track)

    return handle_resource_broker_async_method_success(
        m, ret_error, "us", out_cc_reg_status, out_access_token.c_str());
}

int DBusAdapterImpl::process_message_DeregisterResources(sd_bus_message* m, sd_bus_error* ret_error)
{
    TR_DEBUG("Enter");
    assert(m);
    TR_INFO("Starting to process DergisterResources method call from sender %s",
            sd_bus_message_get_sender(m));

    std::string access_token;
    int r = verify_signature_and_get_string_argument(m, ret_error, access_token);
    if (r < 0) {
        TR_ERR("verify_signature_and_get_string_argument failed!");
        return r;
    }

    // call deregister_resources resource broker APi and handle output
    CloudConnectStatus out_cc_dereg_status = ERR_FAILED;
    MblError mbl_dereg_err = ccrb_.deregister_resources(
        reinterpret_cast<uintptr_t>(m), access_token, out_cc_dereg_status);

    if (MblError::None != mbl_dereg_err || is_CloudConnectStatus_error(out_cc_dereg_status)) {
        return handle_resource_broker_method_failure(
            mbl_dereg_err, out_cc_dereg_status, "deregister_resources", ret_error);
    }

    return handle_resource_broker_async_method_success(m, ret_error, "u", out_cc_dereg_status);
}

MblError DBusAdapterImpl::init()
{
    TR_DEBUG("Enter");
    MblError status = MblError::Unknown;

    if (state_.is_not_equal(DBusAdapterState::UNINITALIZED)) {
        TR_ERR("Unexpected state (expected %s), returning error %s",
               state_.to_string(),
               MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }

    // record initializer thread ID, that should be CCRB main thread
    initializer_thread_id_ = pthread_self();

    // set callback into DBusService C module
    DBusService_init(DBusAdapterImpl::incoming_bus_message_callback);

    // Init incoming message mailbox
    status = mailbox_in_.init();
    if (status != MblError::None) {
        // mailbox deinit itself
        TR_ERR("mailbox_in_.init() failed with error %s", MblError_to_str(status));
        return status;
    }

    status = event_loop_init();
    if (status != MblError::None) {
        // event loop is not an object, need to deinit
        TR_ERR("event_loop_init() failed with error %s", MblError_to_str(status));
        event_loop_deinit();
        return status;
    }

    status = bus_init();
    if (status != Error::None) {
        // bus is not an object, need to deinit
        TR_ERR("bus_init() failed with error %s", MblError_to_str(status));
        bus_deinit();
        return status;
    }

    status = event_manager_.init();
    if (status != MblError::None) {
        TR_ERR("event_manager_.init() failed with error %s", MblError_to_str(status));
        event_manager_.deinit();
        return status;
    }

    state_.set(DBusAdapterState::INITALIZED);
    TR_INFO("init finished with SUCCESS!");
    return Error::None;
}

MblError DBusAdapterImpl::deinit()
{
    TR_DEBUG("Enter");
    OneSetMblError one_set_status;
    MblError status;

    // Must be first! only CCRB initializer thread should call this function.
    assert(pthread_equal(pthread_self(), initializer_thread_id_) != 0);

    if (state_.is_not_equal(DBusAdapterState::INITALIZED)) {
        TR_ERR("Unexpected state (expected %s), returning error %s",
               state_.to_string(),
               MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }

    // Perform a "best effort" deinit - continue on failure and return first error code (if happen)
    status = mailbox_in_.deinit();
    if (status != MblError::None) {
        one_set_status.set(status);
        TR_ERR("mailbox_in_.deinit failed with error %s", MblError_to_str(status));
        // continue
    }

    status = bus_deinit();
    if (status != MblError::None) {
        one_set_status.set(status);
        TR_ERR("bus_deinit() failed with error %s", MblError_to_str(status));
        // continue
    }

    status = event_loop_deinit();
    if (status != MblError::None) {
        one_set_status.set(status);
        TR_ERR("mailbox_in_.deini()) failed with error %s", MblError_to_str(status));
        // continue
    }

    status = event_manager_.deinit();
    if (status != MblError::None) {
        one_set_status.set(status);
        TR_ERR("event_manager_.deinit() failed with error %s", MblError_to_str(status));
        // continue
    }

    DBusService_deinit();

    state_.set(DBusAdapterState::UNINITALIZED);
    if (MblError::None == one_set_status.get()) {
        TR_INFO("Deinit finished with SUCCESS!");
    }
    return one_set_status.get();
}

MblError DBusAdapterImpl::event_loop_run(MblError& stop_status)
{
    TR_DEBUG("Enter - Start running!");

    /*
    Thread enters the sd-event loop and blocks. See:
    https://www.freedesktop.org/software/systemd/man/sd_event_run.html# :
    sd_event_loop() invokes sd_event_run() in a loop,
    thus implementing the actual event loop. The call returns as soon as exiting
    was requested using sd_event_exit(3).
    sd_event_loop() returns the exit code specified when invoking sd_event_exit()
    */
    state_.set(DBusAdapterState::RUNNING);
    stop_status = (MblError) sd_event_loop(event_loop_handle_);
    state_.set(DBusAdapterState::INITALIZED);
    return MblError::None;
}

MblError DBusAdapterImpl::run(MblError& stop_status)
{
    TR_DEBUG("Enter");
    MblError status = MblError::Unknown;

    // Must be first! only CCRB initializer thread should call this function.
    assert(pthread_equal(pthread_self(), initializer_thread_id_) != 0);

    if (state_.is_not_equal(DBusAdapterState::INITALIZED)) {
        TR_ERR("Unexpected state (expected %s), returning error %s",
               state_.to_string(),
               MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }

    status = event_loop_run(stop_status);
    if (status != MblError::None) {
        TR_ERR("event_loop_run() failed with error %s", MblError_to_str(status));
        return status;
    }
    state_.set(DBusAdapterState::INITALIZED);

    return MblError::None;
}

MblError DBusAdapterImpl::stop(MblError stop_status)
{
    TR_DEBUG("Enter");
    MblError status = MblError::Unknown;

    if (state_.is_equal(DBusAdapterState::UNINITALIZED)) {
        TR_ERR("Unexpected state (expected %s), returning error %s",
               state_.to_string(),
               MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }

    if (pthread_equal(pthread_self(), initializer_thread_id_) != 0) {
        // This section is for self exit request, use event_loop_request_stop
        // current thread id ==  initializer_thread_id_
        int r = event_loop_request_stop(stop_status);
        if (r < 0) {
            status = MblError::DBA_SdEventExitRequestFailure;
            TR_ERR("event_loop_request_stop() failed with error %s (r=%d) - set error %s",
                   strerror(r),
                   r,
                   MblError_to_str(MblError::DBA_SdEventExitRequestFailure));
            // continue
        }
        else
        {
            TR_INFO("Sent self request to exit sd-event loop!");
        }
    }
    else
    {
        // This section is for external threads exit requests - send EXIT message to mailbox_in_
        // Thread shouldn't block here, but we still supply a maximum timeout of
        // MSG_SEND_ASYNC_TIMEOUT_MILLISECONDS
        MailboxMsg::MsgPayload payload;
        payload.exit_.stop_status = stop_status;
        MailboxMsg msg(MailboxMsg::MsgType::EXIT, payload, sizeof(payload.exit_));

        status = mailbox_in_.send_msg(msg);
        if (status != MblError::None) {
            TR_ERR("mailbox_in_.send_msg failed with error %s", MblError_to_str(status));
            status = MblError::DBA_SdEventCallFailure;
            // continue
        }
        else
        {
            TR_INFO("Sent request to stop CCRB thread inside sd-event loop!");
        }
    }

    return status;
}

/**
 * TODO: The D-Bus signal functionality is tested manually and not tested by gtests.
 * Need to add relevant gtests. See: IOTMBL-1691
 */
MblError DBusAdapterImpl::handle_resource_broker_async_process_status_update(
    const uintptr_t ipc_request_handle, const char* signal_name, const CloudConnectStatus status)
{
    tr_debug("Enter");
    assert(ipc_request_handle);
    assert(signal_name);

    if (state_.is_not_equal(DBusAdapterState::RUNNING)) {
        TR_ERR("Unexpected state (expected %s), returning error %s",
               state_.to_string(),
               MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }

    sd_bus_message* m_to_signal_on = reinterpret_cast<sd_bus_message*>(ipc_request_handle);

    // try erase handle from pending_messages
    size_t deleted_items_num = pending_messages_.erase(m_to_signal_on);
    if (0 == deleted_items_num) {
        // handle provided was not previously stored
        TR_ERR("provided handle (0x%" PRIxPTR ") not found in pending messages, returning error %s",
               ipc_request_handle,
               MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }

    // m_to_signal_on's refcount should be reduces when the flow leave this function
    sd_objects_cleaner<sd_bus_message> message_cleaner(m_to_signal_on, sd_bus_message_unref);

    if (1 < deleted_items_num) {
        // handle provided was stored somehow more than one time!
        TR_ERR("provided handle (0x%" PRIxPTR ") found more than once in pending messages, "
               "returning error %s",
               ipc_request_handle,
               MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }

    sd_bus_message* signal = nullptr;
    sd_objects_cleaner<sd_bus_message> signal_cleaner(signal, sd_bus_message_unref);

    int r = sd_bus_message_new_signal(connection_handle_,
                                      &signal,
                                      DBUS_CLOUD_CONNECT_OBJECT_PATH,
                                      DBUS_CLOUD_CONNECT_INTERFACE_NAME,
                                      signal_name);
    if (r < 0) {
        TR_ERR("sd_bus_message_new_signal name=%s failed(err=%d)", signal_name, r);
        return MblError::DBA_SdBusCallFailure;
    }

    // set destination of signal message
    const char* signal_dest = sd_bus_message_get_sender(m_to_signal_on);
    r = sd_bus_message_set_destination(signal, signal_dest);
    if (r < 0) {
        TR_ERR("sd_bus_message_set_destination dest=%s failed(err=%d)", signal_dest, r);
        return MblError::DBA_SdBusCallFailure;
    }

    // append status
    r = sd_bus_message_append(signal, "u", status);
    if (r < 0) {
        TR_ERR("sd_bus_message_append failed(err=%d)", r);
        return MblError::DBA_SdBusCallFailure;
    }

    r = sd_bus_send(connection_handle_, signal, nullptr);
    if (r < 0) {
        TR_ERR("sd_bus_send dest=%s failed(err=%d)", signal_dest, r);
        return MblError::DBA_SdBusCallFailure;
    }

    tr_debug("Signal %s was successfully emitted to %s", signal_name, signal_dest);

    return MblError::None;
}

bool DBusAdapterImpl::is_valid_message_type(uint8_t message_type)
{
    switch (message_type)
    {
    case SD_BUS_MESSAGE_METHOD_CALL: return true;
    case SD_BUS_MESSAGE_METHOD_RETURN: return true;
    case SD_BUS_MESSAGE_METHOD_ERROR: return true;
    case SD_BUS_MESSAGE_SIGNAL: return true;
    default: return false;
    }
}

// https://www.freedesktop.org/software/systemd/man/sd_bus_message_get_type.html#
const char* DBusAdapterImpl::message_type_to_str(uint8_t message_type)
{
    switch (message_type)
    {
    case SD_BUS_MESSAGE_METHOD_CALL: return stringify(SD_BUS_MESSAGE_METHOD_CALL);
    case SD_BUS_MESSAGE_METHOD_RETURN: return stringify(SD_BUS_MESSAGE_METHOD_RETURN);
    case SD_BUS_MESSAGE_METHOD_ERROR: return stringify(SD_BUS_MESSAGE_METHOD_ERROR);
    case SD_BUS_MESSAGE_SIGNAL: return stringify(SD_BUS_MESSAGE_SIGNAL);
    default: return "UNKNOWN SD_BUS MESSAGE TYPE!";
    }
}

const char* DBusAdapterImpl::State::to_string()
{
    switch (current_)
    {
    case eState::UNINITALIZED: return "UNINITALIZED";
    case eState::INITALIZED: return "INITALIZED";
    case eState::RUNNING: return "RUNNING";
    default:
        assert(0); // should never happen!
        return "UNKNOWN STATE";
    }
}

void DBusAdapterImpl::State::set(eState new_state)
{
    if (current_ != new_state) {
        current_ = new_state;
        TR_INFO("New adapter state %s", to_string());
    }
}

DBusAdapterImpl::DBusAdapterState DBusAdapterImpl::State::get()
{
    return current_;
}

bool DBusAdapterImpl::State::is_equal(eState state)
{
    return (current_ == state);
}

bool DBusAdapterImpl::State::is_not_equal(eState state)
{
    return (current_ != state);
}

MblError DBusAdapterImpl::send_event_immediate(SelfEvent::EventData data,
                                               unsigned long data_length,
                                               SelfEvent::EventDataType data_type,
                                               SelfEventCallback callback,
                                               uint64_t& out_event_id,
                                               const std::string description)
{
    TR_DEBUG("Enter");
    assert(callback);

    // Must be first! only CCRB initializer thread should call this function.
    assert(pthread_equal(pthread_self(), initializer_thread_id_) != 0);

    return event_manager_.send_event_immediate(
        data, data_length, data_type, callback, out_event_id, description);
}

} // namespace mbl {
