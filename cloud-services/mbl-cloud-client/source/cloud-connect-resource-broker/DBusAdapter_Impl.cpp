/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "DBusAdapter_Impl.h"
#include "CloudConnectTrace.h"
#include "CloudConnectTypes.h"
#include "DBusAdapter.h"
#include "DBusAdapterCommon.h"
#include "DBusCloudConnectNames.h"
#include "DBusMessage.h"
#include "DBusMessagesFactory.h"
#include "DBusService.h"
#include "ResourceBroker.h"

#include <systemd/sd-id128.h>

#include <cassert>
#include <cstdbool>
#include <sstream>
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
    TR_DEBUG_ENTER;
    state_.set(DBusAdapterState::UNINITALIZED);
}

// Cloud Connect errors to D-Bus format error strings translation map.
// Information from this map is used when D-Bus ifrastructure translates
// sd_bus_error.name field string to the negative integer value.
static constexpr sd_bus_error_map cloud_connect_dbus_errors[] = {
    SD_BUS_ERROR_MAP(CLOUD_CONNECT_ERR_INTERNAL_ERROR, ERR_INTERNAL_ERROR),
    SD_BUS_ERROR_MAP(CLOUD_CONNECT_ERR_INVALID_APPLICATION_RESOURCES_DEFINITION,
                     ERR_INVALID_APPLICATION_RESOURCES_DEFINITION),
    SD_BUS_ERROR_MAP(CLOUD_CONNECT_ERR_REGISTRATION_ALREADY_IN_PROGRESS,
                     ERR_REGISTRATION_ALREADY_IN_PROGRESS),
    SD_BUS_ERROR_MAP(CLOUD_CONNECT_ERR_ALREADY_REGISTERED, ERR_ALREADY_REGISTERED),
    SD_BUS_ERROR_MAP(CLOUD_CONNECT_ERR_INVALID_RESOURCE_PATH, ERR_INVALID_RESOURCE_PATH),
    SD_BUS_ERROR_MAP(CLOUD_CONNECT_ERR_RESOURCE_NOT_FOUND, ERR_RESOURCE_NOT_FOUND),
    SD_BUS_ERROR_MAP(CLOUD_CONNECT_ERR_INVALID_RESOURCE_TYPE, ERR_INVALID_RESOURCE_TYPE),
    SD_BUS_ERROR_MAP(CLOUD_CONNECT_ERR_INVALID_ACCESS_TOKEN, ERR_INVALID_ACCESS_TOKEN),
    SD_BUS_ERROR_MAP(CLOUD_CONNECT_ERR_NUM_ALLOWED_CONNECTIONS_EXCEEDED,
                     ERR_NUM_ALLOWED_CONNECTIONS_EXCEEDED),

    // must be always the last
    SD_BUS_ERROR_MAP_END};

MblError DBusAdapterImpl::bus_init()
{
    TR_DEBUG_ENTER;

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
               strerror(-r),
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
               strerror(-r),
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
               strerror(-r),
               MblError_to_str(MblError::DBA_SdBusCallFailure));
        return MblError::DBA_SdBusCallFailure;
    }
    TR_INFO("Added new interface %s using service_vtable to object %s",
            DBUS_CLOUD_CONNECT_INTERFACE_NAME,
            DBUS_CLOUD_CONNECT_OBJECT_PATH);

    // set callback into DBusService C module
    DBusService_init(DBusAdapterImpl::incoming_bus_message_callback);

    // Get my unique name on the bus
    r = sd_bus_get_unique_name(connection_handle_, &unique_name_);
    if (r < 0) {
        TR_ERR("sd_bus_get_unique_name failed with error r=%d (%s) - returning %s",
               r,
               strerror(-r),
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
               strerror(-r),
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
               strerror(-r),
               MblError_to_str(MblError::DBA_SdBusCallFailure));
        return MblError::DBA_SdBusCallFailure;
    }

    return MblError::None;
}

int DBusAdapterImpl::disconnected_bus_name_callback(sd_bus_track* track, void* userdata)
{
    TR_DEBUG_ENTER;
    assert(track);
    assert(userdata);

    DBusAdapterImpl* adapter_impl = static_cast<DBusAdapterImpl*>(userdata);
    return adapter_impl->disconnected_bus_name_callback_impl(track);
}

int DBusAdapterImpl::disconnected_bus_name_callback_impl(sd_bus_track* track)
{
    TR_DEBUG_ENTER;
    assert(track);

    // sanity check 2 - look for the sd_bus_track object inside the map, and remove it.
    // It should be always found
    auto iter = connections_tracker_.begin();
    for (; iter != connections_tracker_.end(); ++iter) {
        if (iter->second == track) {
            break;
        }
    }
    if (iter == connections_tracker_.end()) {
        TR_ERR("track object not found in connections_tracker_!returning error -EBADF");
        return -EBADF;
    }
    std::string disconnected_bus_name = iter->first;
    connections_tracker_.erase(iter);

    // unref track object
    sd_bus_track_unref(track);

    TR_INFO("bus name=%s disconnected! Informing CCRB!", disconnected_bus_name.c_str());
    bus_track_debug();

    ccrb_.notify_connection_closed(IpcConnection(disconnected_bus_name));
    return 0;
}

void DBusAdapterImpl::bus_track_debug()
{
    if (!connections_tracker_.empty()) {
        std::stringstream log_msg;
        log_msg << "Currently tracked names :";

        for (auto& pair : connections_tracker_) {
            log_msg << " " << pair.first;
        }
        TR_DEBUG("%s", log_msg.str().c_str());
    }
    else
    {
        TR_DEBUG("There are currently no tracked names!");
    }
}

int DBusAdapterImpl::bus_track_sender(const char* bus_name)
{
    TR_DEBUG_ENTER;
    assert(bus_name);

    auto iter = connections_tracker_.find(bus_name);
    if (connections_tracker_.end() == iter) {
        // Not found, add a new tracing object
        sd_bus_track* track = nullptr;
        int r = sd_bus_track_new(connection_handle_, &track, disconnected_bus_name_callback, this);
        if (r < 0) {
            TR_ERR("sd_bus_track_new failed with error r=%d (%s)", r, strerror(-r));
            return r;
        }

        // track bus_name
        // man page: "return 0 if the specified name has already been added to the bus peer tracking
        // object before and positive if it hasn't"
        r = sd_bus_track_add_name(track, bus_name);
        if (0 == r) {
            // should never happen
            sd_bus_track_unref(track);
            TR_ERR("sd_bus_track_add_name failed - name already added? (that can't happen!) "
                   " returning -EFAULT");
            return -EFAULT;
        }
        if (r < 0) {
            TR_ERR("sd_bus_track_add_name failed with error r=%d (%s)", r, strerror(-r));
            return r;
        }

        // Now add pair of bus_name and track object
        connections_tracker_.insert(make_pair(bus_name, track));

        TR_INFO("bus_name=%s is tracked! (added to connections_tracker_)", bus_name);
        // print debug information
        bus_track_debug();
    }
    return 0;
}

MblError DBusAdapterImpl::bus_deinit()
{
    TR_DEBUG_ENTER;

    // sd_bus_flush_close_unref return always nullptr
    connection_handle_ = sd_bus_flush_close_unref(connection_handle_);
    service_name_ = nullptr;
    unique_name_ = nullptr;

    DBusService_deinit();

    for (auto& pair : connections_tracker_) {
        sd_bus_track_unref(pair.second);
    }

    return MblError::None;
}

MblError DBusAdapterImpl::event_loop_init()
{
    TR_DEBUG_ENTER;

    // Create the sd-event loop object (thread loop)
    int r = sd_event_default(&event_loop_handle_);
    if (r < 0) {
        TR_ERR("sd_event_default failed with error r=%d (%s) - returning %s",
               r,
               strerror(-r),
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
               strerror(-r),
               MblError_to_str(MblError::DBA_SdEventCallFailure));
        return MblError::DBA_SdEventCallFailure;
    }
    TR_INFO("Added floating IO (input) event source to attach output from mailbox)");

    return MblError::None;
}

MblError DBusAdapterImpl::event_loop_deinit()
{
    TR_DEBUG_ENTER;

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
    TR_DEBUG_ENTER;

    // Must be first! only CCRB initializer thread should call this function.
    assert(pthread_equal(pthread_self(), initializer_thread_id_) != 0);

    // Send myself an exit request
    int r = sd_event_exit(event_loop_handle_, (int) stop_status);
    if (r < 0) {
        TR_ERR("sd_event_exit failed with error r=%d (%s)", r, strerror(-r));
    }
    else
    {
        TR_INFO("sd_event_exit called with stop_status=%d", (int) stop_status);
    }
    return r;
}

int DBusAdapterImpl::incoming_mailbox_message_callback_impl(sd_event_source* s,
                                                            int fd,
                                                            uint32_t revents)
{
    assert(s);
    TR_DEBUG_ENTER;
    UNUSED(s);

    // Validate that revents contains epoll read event flag
    if ((revents & EPOLLIN) == 0) {
        // TODO: not sure if this error is possible - if it is -
        // we need to restart thread/process or target (??)

        TR_ERR("(revents & EPOLLIN == 0), returning -EBADFD to  disable event source");
        return (-EBADFD);
    }

    // Another validation - given fd is the one belongs to the mailbox (input side)
    if (fd != mailbox_in_.get_pipefd_read()) {
        // TODO: handle on upper layer - need to notify somehow
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
    const MblError status = ccrb_.process_mailbox_message(ret_pair.second);
    if(Error::DBA_MailBoxInvalidMsg == status) {
        TR_ERR("process_mailbox_message failed with error: %s", MblError_to_str(status));
        return (-EBADMSG);
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
    TR_DEBUG_ENTER;
    DBusAdapterImpl* adapter_impl = static_cast<DBusAdapterImpl*>(userdata);
    return adapter_impl->incoming_mailbox_message_callback_impl(s, fd, revents);
}

// TODO - This is a first stage enforcement function - allow a single connection to interact with
// our service. to be remove later on.
bool DBusAdapterImpl::bus_enforce_single_connection(std::string& source)
{
    TR_DEBUG_ENTER;

    if (connections_tracker_.empty()) {
        // no connection is tracked - continue processing arriving message
        return true;
    }

    if (connections_tracker_.find(source.c_str()) != connections_tracker_.end()) {
        // this sender is tracked, and is the only allowed connection until disconnection - continue
        // processing arriving message
        return true;
    }

    // If we are here, some unique connection ID is tracked already and it is not the sender -
    // reply with error and return false to caller to stop processing
    TR_ERR("Message from source=%s cannot be processed! Only a single connection is allowed!",
           source.c_str());

    return false;
}

int DBusAdapterImpl::incoming_bus_message_callback(sd_bus_message* m,
                                                   void* userdata,
                                                   sd_bus_error* ret_error)
{
    TR_DEBUG_ENTER;
    assert(userdata);
    assert(m);
    assert(ret_error);
    *ret_error = SD_BUS_ERROR_NULL;

    // TODO: For all failues here, might need to send an error reply ONLY if the message is of
    // kind method_call (can check that) check what is done in other implementations
    // see https://www.freedesktop.org/software/systemd/man/sd_bus_message_get_type.html#

    // TODO: as of now, we do not expect for this callback any empty message.
    // In the future - if there are any empty messages - need to handle them here
    // (for now only standard)
    if (sd_bus_message_is_empty(m) != 0) {
        std::stringstream msg("Received an empty message!");
        return LOG_AND_SET_SD_BUS_ERROR_F(EBADMSG, ret_error, msg);
    }

    // Expect message with our known name, directly sent to us (unicast)
    if (0 != strncmp(sd_bus_message_get_destination(m),
                     DBUS_CLOUD_SERVICE_NAME,
                     strlen(DBUS_CLOUD_SERVICE_NAME)))
    {
        std::stringstream msg("Received message to wrong destination " +
                              std::string(sd_bus_message_get_destination(m)));
        return LOG_AND_SET_SD_BUS_ERROR_F(EFAULT, ret_error, msg);
    }

    // Expect message to a single object path DBUS_CLOUD_CONNECT_OBJECT_PATH
    if (0 != strncmp(sd_bus_message_get_path(m),
                     DBUS_CLOUD_CONNECT_OBJECT_PATH,
                     strlen(DBUS_CLOUD_CONNECT_OBJECT_PATH)))
    {
        std::stringstream msg("Unexisting object path " + std::string(sd_bus_message_get_path(m)));
        return LOG_AND_SET_SD_BUS_ERROR_F(EFAULT, ret_error, msg);
    }

    // Expect message to a single interface DBUS_CLOUD_CONNECT_INTERFACE_NAME
    if (0 != strncmp(sd_bus_message_get_interface(m),
                     DBUS_CLOUD_CONNECT_INTERFACE_NAME,
                     strlen(DBUS_CLOUD_CONNECT_INTERFACE_NAME)))
    {
        std::stringstream msg("Unexisting interface " +
                              std::string(sd_bus_message_get_interface(m)));
        return LOG_AND_SET_SD_BUS_ERROR_F(EFAULT, ret_error, msg);
    }

    /*
    At this stage we are sure to handle all messages to known service DBUS_CLOUD_SERVICE_NAME,
    Object DBUS_CLOUD_CONNECT_OBJECT_PATH and interface DBUS_CLOUD_CONNECT_INTERFACE_NAME
    Messages can be of type Signal / Error / Method Call.
    */
    uint8_t type = 0;
    int r = sd_bus_message_get_type(m, &type);
    if (r < 0) {
        return LOG_AND_SET_SD_BUS_ERROR(r, ret_error, "sd_bus_message_get_type");
    }
    if (!(is_valid_message_type(type))) {
        std::stringstream msg("Received message from a wrong type " +
                              std::string(message_type_to_str(type)));
        return LOG_AND_SET_SD_BUS_ERROR_F(ENOMSG, ret_error, msg);
    }

    const char* csender = sd_bus_message_get_sender(m);
    if (nullptr == csender) {
        std::stringstream msg("Unknown SENDER field on message header is not allowed!");
        return LOG_AND_SET_SD_BUS_ERROR_F(EINVAL, ret_error, msg);
    }
    std::string sender(csender);

    // sender fields must be non-empty , must start with with colon and its length must be
    // larger than 1,
    if ((sender.size() <= 1) || (sender[0] != ':')) {
        TR_ERR("Invalid sender=[%s] (sender connection ID must be at least 2 characters and start"
               " with a colon) - returning -EFAULT",
               sender.c_str());
        return LOG_AND_SET_SD_BUS_ERROR(-EFAULT, ret_error, __func__);
    }
    TR_INFO(
        "Received message of type %s from sender %s", message_type_to_str(type), sender.c_str());

    // Add sender to tracked connections (if not tracked already).

    // TODO - At this stage of development, we allow only a single connection to communicate with
    // our service at any specific time. We believe that multiple connections will probably work,
    // but this hasn't been tested probably yet
    DBusAdapterImpl* impl = static_cast<DBusAdapterImpl*>(userdata);
    if (!impl->bus_enforce_single_connection(sender)) {
        CloudConnectStatus status = ERR_NUM_ALLOWED_CONNECTIONS_EXCEEDED;
        TR_ERR("Only single connection allowed! (%s is not the active connection)"
               " returning to sender CloudConnectStatus=%s",
               sender.c_str(),
               CloudConnectStatus_to_str(status));

        r = sd_bus_error_set_const(ret_error,
                                   CloudConnectStatus_error_to_DBus_format_str(status),
                                   CloudConnectStatus_to_readable_str(status));
        assert(-status == r);
        return r;
    }

    r = impl->bus_track_sender(sender.c_str());
    if (r < 0) {
        return LOG_AND_SET_SD_BUS_ERROR(r, ret_error, "bus_track_sender");
    }

    if (sd_bus_message_get_expect_reply(m) == 0) {
        // reply to the message m is not expected.
        std::stringstream msg("Unexpected message type: no reply expected!");
        return LOG_AND_SET_SD_BUS_ERROR_F(ENOMSG, ret_error, msg);
    }

    const char* method_name = sd_bus_message_get_member(m);
    if (method_name == nullptr) {
        std::stringstream msg("sd_bus_message_get_member returned NULL!");
        return LOG_AND_SET_SD_BUS_ERROR_F(EINVAL, ret_error, msg);
    }

    const char* sender_name = sd_bus_message_get_sender(m);
    if (sender_name == nullptr) {
        std::stringstream msg("sd_bus_message_get_sender returned NULL!");
        return LOG_AND_SET_SD_BUS_ERROR_F(EINVAL, ret_error, msg);
    }

    std::shared_ptr<DBusCommonMessageProcessor> message_processor =
        DBusMessagesFactory::get_message_processor(method_name);
    if (message_processor == nullptr) {

        TR_ERR("Failed to find message processor for message=%s", method_name);

        r = sd_bus_error_set_const(
            ret_error,
            CloudConnectStatus_error_to_DBus_format_str(ERR_NOT_SUPPORTED), // sd_bus_error.name
            CloudConnectStatus_to_readable_str(ERR_NOT_SUPPORTED));         // sd_bus_error.message
        assert(-ERR_NOT_SUPPORTED == r);
        return r;
    }

    TR_INFO("Starting to process %s method call from sender %s", method_name, sender_name);

    r = message_processor->process_message(
        impl->get_connection_handle(), m, impl->get_ccrb(), ret_error);
    if (r < 0) {
        TR_ERR("process_message failed, r=%d", r);
    }

    std::stringstream log_msg;
    log_msg << "Message of type " << message_type_to_str(type) << " member name " << sender_name
            << " from sender " << sender_name << " - ";

    // handle and print to logs handling status
    if (r == 0) {
        log_msg << "SUCCESSFULLY processed";
    }
    else
    {
        log_msg << "FAILED to process: ";
        if (r > 0) {
            // positive value of r is unexpected
            log_msg << "unexpected grater than 0 r=" << r << " (returning as negative error " << -r
                    << ")";
            r = (-r); // return negative status
        }
        else
        {
            // r is negative, that means failure
            log_msg << "failure status r=" << r;
        }
    }

    TR_DEBUG("%s", log_msg.str().c_str());

    return r;
}

std::pair<MblError, std::string> DBusAdapterImpl::generate_access_token()
{
    TR_DEBUG_ENTER;

    std::pair<MblError, std::string> ret_pair(MblError::None, std::string());

    sd_id128_t id128 = SD_ID128_NULL;
    int retval = sd_id128_randomize(&id128);
    if (retval != 0) {
        TR_ERR("sd_id128_randomize failed with error: %d", retval);
        ret_pair.first = Error::CCRBGenerateUniqueIdFailed;
        return ret_pair;
    }

    // Add a char for NULL termination
    char buffer[SD_ID_128_UNIQUE_ID_LEN + 1] = {0};

    ret_pair.second = sd_id128_to_string(id128, buffer);
    return ret_pair;
}

MblError DBusAdapterImpl::init()
{
    TR_DEBUG_ENTER;
    MblError status = MblError::Unknown;

    if (state_.is_not_equal(DBusAdapterState::UNINITALIZED)) {
        TR_ERR("Unexpected state (expected %s), returning error %s",
               state_.to_string(),
               MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }

    // record initializer thread ID, that should be CCRB main thread
    initializer_thread_id_ = pthread_self();

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
    TR_DEBUG_ENTER;
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
    TR_DEBUG_ENTER;
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
    TR_DEBUG_ENTER;

    if (state_.is_equal(DBusAdapterState::UNINITALIZED)) {
        TR_ERR("Unexpected state (expected %s), returning error %s",
               state_.to_string(),
               MblError_to_str(MblError::DBA_IllegalState));
        return MblError::DBA_IllegalState;
    }

    // This API is allowed to be called only from internal (initializer) thread
    assert(pthread_equal(pthread_self(), initializer_thread_id_) != 0);

    // Self exit request, use event_loop_request_stop 
    int r = event_loop_request_stop(stop_status);
    if (r < 0) {
        TR_ERR("event_loop_request_stop() failed with error %s (r=%d) - set error %s",
                strerror(-r),
                r,
                MblError_to_str(MblError::DBA_SdEventExitRequestFailure));
        return MblError::DBA_SdEventExitRequestFailure;
    }
    TR_INFO("Sent self request to exit sd-event loop!");
    return MblError::None;
}

/**
 * TODO: The D-Bus signal functionality is tested manually and not tested by gtests.
 * Need to add relevant gtests. See: IOTMBL-1691
 */
MblError DBusAdapterImpl::handle_resource_broker_async_process_status_update(
    const IpcConnection& /*unused*/, const char* signal_name, const CloudConnectStatus status)
{
    TR_DEBUG_ENTER;
    assert(signal_name);
    TR_DEBUG("signal_name = %s, status = %s", signal_name, CloudConnectStatus_to_str(status));

    // Temporary disable sending of the signal to the destination application before we have
    // IOTMBL-1691 done. In order to send signals to the application just enable this code:
    // if (state_.is_not_equal(DBusAdapterState::RUNNING)) {
    //     TR_ERR("Unexpected state (expected %s), returning error %s",
    //            state_.to_string(),
    //            MblError_to_str(MblError::DBA_IllegalState));
    //     return MblError::DBA_IllegalState;
    // }

    // sd_bus_message* m_signal = nullptr;
    // sd_bus_object_cleaner<sd_bus_message> signal_cleaner(m_signal, sd_bus_message_unref);

    // int r = sd_bus_message_new_signal(connection_handle_,
    //                                   &m_signal,
    //                                 DBUS_CLOUD_CONNECT_OBJECT_PATH,
    //                                   DBUS_CLOUD_CONNECT_INTERFACE_NAME,
    //                                   signal_name);
    // if (r < 0) {
    //     TR_ERRNO_F("sd_bus_message_new_signal", r, "(signal name=%s)", signal_name);
    //     return MblError::DBA_SdBusCallFailure;
    // }

    // set destination of signal message

    // const char* signal_dest = destination_to_update.get_connection_id().c_str();

    // r = sd_bus_message_set_destination(m_signal, signal_dest);
    // if (r < 0) {
    //     TR_ERRNO_F("sd_bus_message_set_destination", r, "(signal destination=%s)", signal_dest);
    //     return MblError::DBA_SdBusCallFailure;
    // }

    // append status
    // r = sd_bus_message_append(m_signal, "u", status);
    // if (r < 0) {
    //     TR_ERRNO_F("sd_bus_message_append", r, "(signal types = %s)", "u");
    //     return MblError::DBA_SdBusCallFailure;
    // }

    //    r = sd_bus_send(connection_handle_, m_signal, nullptr);
    //    if (r < 0) {
    //        TR_ERRNO_F("sd_bus_send", r, "(signal destination=%s)", signal_dest);
    //        return MblError::DBA_SdBusCallFailure;
    //    }

    // TR_DEBUG("Signal %s was successfully emitted to %s", signal_name, signal_dest);

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

MblError DBusAdapterImpl::send_mailbox_msg(MailboxMsg& msg_to_send)
{
    TR_DEBUG_ENTER;

    MblError status = mailbox_in_.send_msg(msg_to_send);
    if (status != MblError::None) {
        TR_ERR("mailbox_in_.send_msg failed with error %s", MblError_to_str(status));
        return MblError::DBA_SdEventCallFailure;
    }
    return status;
}


} // namespace mbl
