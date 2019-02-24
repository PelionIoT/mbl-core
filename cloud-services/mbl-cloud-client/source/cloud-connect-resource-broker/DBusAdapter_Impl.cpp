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
        TR_ERR("sd_event_exit failed with error r=%d (%s) - returning %s",
               r,
               strerror(r),
               status.get_status_str());
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
        TR_ERR("mailbox_in_.receive_msg failed with status=%s, disable event source!",
               MblError_to_str(status));
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
            TR_ERR("event_loop_request_stop() failed with error %s (r=%d)")
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
        TR_ERR("Received an empty message!");
        return (-EINVAL);
    }

    // Expect message with our known name, directly sent to us (unicast)
    if (0 != strncmp(sd_bus_message_get_destination(m),
                     DBUS_CLOUD_SERVICE_NAME,
                     strlen(DBUS_CLOUD_SERVICE_NAME)))
    {
        TR_ERR("Received message to wrong destination (%s)!", sd_bus_message_get_destination(m));
        return (-EINVAL);
    }

    // Expect message to a single object patch DBUS_CLOUD_CONNECT_OBJECT_PATH
    if (0 != strncmp(sd_bus_message_get_path(m),
                     DBUS_CLOUD_CONNECT_OBJECT_PATH,
                     strlen(DBUS_CLOUD_CONNECT_OBJECT_PATH)))
    {
        TR_ERR("Unexisting object path (%s)!", sd_bus_message_get_path(m));
        return (-EINVAL);
    }

    // Expect message to a single interface DBUS_CLOUD_CONNECT_INTERFACE_NAME
    if (0 != strncmp(sd_bus_message_get_interface(m),
                     DBUS_CLOUD_CONNECT_INTERFACE_NAME,
                     strlen(DBUS_CLOUD_CONNECT_INTERFACE_NAME)))
    {
        TR_ERR("Unexisting interface (%s)!", sd_bus_message_get_interface(m));
        return (-EINVAL);
    }

    /*
    At this stage we are sure to handle all messages to known service DBUS_CLOUD_SERVICE_NAME,
    Object DBUS_CLOUD_CONNECT_OBJECT_PATH and interface DBUS_CLOUD_CONNECT_INTERFACE_NAME
    Messages can be of type Signal / Error / Method Call.
    */
    uint8_t type = 0;
    int r = sd_bus_message_get_type(m, &type);
    if (r < 0) {
        TR_ERR("sd_bus_message_get_type failed with error r=%d (%s)", r, strerror(r));
        return r;
    }
    if (!is_valid_message_type(type)) {
        TR_ERR("Invalid message type %" PRIu8 " returned by sd_bus_message_get_type!", type);
        return (-EINVAL);
    }

    TR_INFO("Received message of type %s from sender %s",
            message_type_to_str(type),
            sd_bus_message_get_sender(m));

    DBusAdapterImpl* impl = static_cast<DBusAdapterImpl*>(userdata);
    if (sd_bus_message_is_method_call(m, nullptr, "RegisterResources") >= 0) {
        r = impl->process_message_RegisterResources(m, ret_error);
        UNUSED(r); // FIXME
        // TODO - handle return value and continue implementation
        assert(0);
    }
    else if (sd_bus_message_is_method_call(m, nullptr, "DeregisterResources") >= 0)
    {
        r = impl->process_message_DeregisterResources(m, ret_error);
        UNUSED(r);
        // TODO - handle return value and continue implementation
        assert(0);
    }
    else
    {
        // TODO - probably need to reply with error reply to sender?
        TR_ERR("Received a message with unknown member=%s!", sd_bus_message_get_member(m));
        assert(0);
        return (-EINVAL);
    }

    return 0;
}

// TODO - this function is incomplete!!
int DBusAdapterImpl::process_message_RegisterResources(sd_bus_message* m, sd_bus_error* ret_error)
{
    assert(ret_error);
    assert(m);
    UNUSED(ret_error);
    TR_DEBUG("Enter");
    const char* json_file_data = nullptr;

    TR_INFO("Starting to process RegisterResources method call from sender %s",
            sd_bus_message_get_sender(m));

    if (sd_bus_message_has_signature(m, "s") != 0) {
        TR_ERR("Unexpected signature %s", sd_bus_message_get_signature(m, 1));
        return (-EINVAL);
    }
    int r = sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &json_file_data);
    if (r < 0) {
        TR_ERR("sd_bus_message_read_basic failed with error r=%d (%s)", r, strerror(r));
        return r;
    }
    if ((nullptr == json_file_data) || (strlen(json_file_data) == 0)) {
        TR_ERR("sd_bus_message_read_basic returned a nullptr or an empty string!");
        return (-EINVAL);
    }

    // Register resources is an asynchronous process towards the cloud -> store handle and
    // increase refcount
    sd_bus_message_ref(m);
    if (!pending_messages_.insert(m).second) {
        TR_ERR("pending_messages_.insert failed!");
        return (-EINVAL);
    }

    // TODO - IOTMBL-1527
    // validate app registered expected interface on bus? (use sd-bus track)

    // TODO - call CCRB
    // ccrb_.register_resources();

    // TODO - handle reply
    assert(0);
    return 0;
}

int DBusAdapterImpl::process_message_DeregisterResources(sd_bus_message* m, sd_bus_error* ret_error)
{
    assert(m);
    assert(ret_error);
    UNUSED(ret_error);
    TR_DEBUG("Enter");
    const char* access_token = nullptr;

    TR_INFO("Starting to process DergisterResources method call from sender %s",
            sd_bus_message_get_sender(m));

    if (sd_bus_message_has_signature(m, "s") != 0) {
        TR_ERR("Unexpected signature %s", sd_bus_message_get_signature(m, 1));
        return (-EINVAL);
    }
    int r = sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &access_token);
    if (r < 0) {
        TR_ERR("sd_bus_message_read_basic failed with error r=%d (%s)", r, strerror(r));
        return r;
    }
    assert(access_token);
    if ((nullptr == access_token) || (strlen(access_token) == 0)) {
        TR_ERR("sd_bus_message_read_basic returned a nullptr or an empty string!");
        return (-EINVAL);
    }

    // Deregister resources is an asynchronous process towards the cloud -> store handle and
    // increase refcount
    sd_bus_message_ref(m);
    if (!pending_messages_.insert(m).second) {
        TR_ERR("pending_messages_.insert failed!");
        return (-EINVAL);
    }

    // TODO call CCRB
    // ccrb_.deregister_resources();

    // TODO - handle reply
    assert(0);
    return 0;
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

    // TODO - make sure we clean all references to messages, timers etc. and unref all sources

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
                   MblError_to_str(DBA_SdEventExitRequestFailure));
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

MblError
DBusAdapterImpl::handle_ccrb_RegisterResources_status_update(const uintptr_t ipc_request_handle,
                                                             const CloudConnectStatus reg_status)
{
    assert(ipc_request_handle != 0);
    // TODO - IMPLEMENT, remove all UNUSED
    UNUSED(ipc_request_handle);
    UNUSED(reg_status);
    TR_DEBUG("Enter");

    // Must be first! only CCRB initializer thread should call this function.
    assert(pthread_equal(pthread_self(), initializer_thread_id_) != 0);

    TR_ERR("CCRB signal updates to application are not yet supported - returning %s",
           MblError_to_str(MblError::DBA_NotSupported));
    return MblError::DBA_NotSupported;

    // This  section is commented to prevent warnings
    // if (state_.is_not_equal(DBusAdapterState::RUNNING)) {
    //     TR_ERR("Unexpected state (expected %s), returning error %s",
    //            state_.to_string(),
    //            MblError_to_str(MblError::DBA_IllegalState));
    //     return MblError::DBA_IllegalState;
    // }

    // return MblError::None;
}

MblError DBusAdapterImpl::handle_ccrb_DeregisterResources_status_update(
    const uintptr_t ipc_request_handle, const CloudConnectStatus dereg_status)
{
    // TODO - IMPLEMENT, remove all UNUSED
    assert(ipc_request_handle != 0);
    UNUSED(ipc_request_handle);
    UNUSED(dereg_status);
    TR_DEBUG("Enter");

    // Must be first! only CCRB initializer thread should call this function.
    assert(pthread_equal(pthread_self(), initializer_thread_id_) != 0);

    TR_ERR("CCRB signal updates to application are not yet supported - returning %s",
           MblError_to_str(MblError::DBA_NotSupported));
    return MblError::DBA_NotSupported;

    // This  section is commented to prevent warnings
    // if (state_.is_not_equal(DBusAdapterState::RUNNING)) {
    //     TR_ERR("Unexpected state (expected %s), returning error %s",
    //            state_.to_string(),
    //            MblError_to_str(MblError::DBA_IllegalState));
    //     return MblError::DBA_IllegalState;
    // }

    // return MblError::None;
}

MblError
DBusAdapterImpl::handle_ccrb_AddResourceInstances_status_update(const uintptr_t ipc_request_handle,
                                                                const CloudConnectStatus add_status)
{
    assert(ipc_request_handle != 0);
    // TODO - IMPLEMENT, remove all UNUSED
    UNUSED(ipc_request_handle);
    UNUSED(add_status);
    TR_DEBUG("Enter");

    // Must be first! only CCRB initializer thread should call this function.
    assert(pthread_equal(pthread_self(), initializer_thread_id_) != 0);

    TR_ERR("CCRB signal updates to application are not yet supported - returning %s",
           MblError_to_str(MblError::DBA_NotSupported));
    return MblError::DBA_NotSupported;

    // This  section is commented to prevent warnings
    // if (state_.is_not_equal(DBusAdapterState::RUNNING)) {
    //     TR_ERR("Unexpected state (expected %s), returning error %s",
    //            state_.to_string(),
    //            MblError_to_str(MblError::DBA_IllegalState));
    //     return MblError::DBA_IllegalState;
    // }
    // return MblError::None;
}

MblError DBusAdapterImpl::handle_ccrb_RemoveResourceInstances_status_update(
    const uintptr_t ipc_request_handle, const CloudConnectStatus remove_status)
{
    assert(ipc_request_handle != 0);
    // TODO - IMPLEMENT, remove all UNUSED
    UNUSED(ipc_request_handle);
    UNUSED(remove_status);
    TR_DEBUG("Enter");

    // Must be first! only CCRB initializer thread should call this function.
    assert(pthread_equal(pthread_self(), initializer_thread_id_) != 0);

    TR_ERR("CCRB signal updates to application are not yet supported - returning %s",
           MblError_to_str(MblError::DBA_NotSupported));
    return MblError::DBA_NotSupported;

    // This  section is commented to prevent warnings
    // if (state_.is_not_equal(DBusAdapterState::RUNNING)) {
    //     TR_ERR("Unexpected state (expected %s), returning error %s",
    //            state_.to_string(),
    //            MblError_to_str(MblError::DBA_IllegalState));
    //     return MblError::DBA_IllegalState;
    // }
    // return MblError::None;
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
