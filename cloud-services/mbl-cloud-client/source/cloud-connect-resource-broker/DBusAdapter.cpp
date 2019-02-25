/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "DBusAdapter.h"
#include "CloudConnectTrace.h"
#include "DBusAdapter_Impl.h"
#include "ResourceBroker.h"

#include <cassert>

#define TRACE_GROUP "ccrb-dbus"

namespace mbl
{

/*
For holding std::unique_ptr<DBusAdapterImpl*>, need to allow unique_ptr to recognize DBusAdapterImpl
as a complete type. std::unique_ptr checks  in its destructor if the definition of the type is
visible before calling delete.
DBusAdapterImpl is forward decleared in header (that a call to delete is well-formed);
std::unique_ptr will refuse to compile and to call delete if the type is only forward declared.
Use the default implentation for the destructor after the definition of DBusAdapterImpl :
*/
DBusAdapter::~DBusAdapter() = default;

DBusAdapter::DBusAdapter(ResourceBroker& ccrb)
    : // initialize impl_ unique_ptr
      impl_(new DBusAdapterImpl(ccrb))
{
    TR_DEBUG("Enter");
}

MblError DBusAdapter::init()
{
    TR_DEBUG("Enter");
    MblError status = impl_->init();
    if (status != MblError::None) {
        TR_DEBUG("impl_->init failed! calling impl_->deinit()");
        impl_->deinit();
    }
    return status;
}

MblError DBusAdapter::deinit()
{
    TR_DEBUG("Enter");
    return impl_->deinit();
}

MblError DBusAdapter::run(MblError& stop_status)
{
    TR_DEBUG("Enter");
    MblError status = impl_->run(stop_status);
    if (status != MblError::None) {
        // best effort - stop
        impl_->stop(status);
    }
    return status;
}

MblError DBusAdapter::stop(MblError stop_status)
{
    TR_DEBUG("Enter");
    return impl_->stop(stop_status);
}

MblError DBusAdapter::update_registration_status(const uintptr_t ipc_request_handle,
                                                 const CloudConnectStatus reg_status)
{
    TR_DEBUG("Enter");
    assert(impl_);
    assert(ipc_request_handle);
    return impl_->handle_resource_broker_async_process_status_update(
        ipc_request_handle, DBUS_CC_REGISTER_RESOURCES_STATUS_SIGNAL_NAME, reg_status);
}

MblError DBusAdapter::update_deregistration_status(const uintptr_t ipc_request_handle,
                                                   const CloudConnectStatus dereg_status)
{
    TR_DEBUG("Enter");
    assert(impl_);
    assert(ipc_request_handle);
    return impl_->handle_resource_broker_async_process_status_update(
        ipc_request_handle, DBUS_CC_DEREGISTER_RESOURCES_STATUS_SIGNAL_NAME, dereg_status);
}

std::pair<MblError, uint64_t> DBusAdapter::send_event_immediate(Event::EventData data,
                                                                unsigned long data_length,
                                                                Event::EventDataType data_type,
                                                                Event::UserCallback callback,
                                                                const std::string& description)
{
    TR_DEBUG("Enter");
    return impl_->send_event_immediate(data, data_length, data_type, callback, description);
}

std::pair<MblError, uint64_t> DBusAdapter::send_event_periodic(Event::EventData data,
                                                               unsigned long data_length,
                                                               Event::EventDataType data_type,
                                                               Event::UserCallback callback,
                                                               uint64_t period_millisec,
                                                               const std::string& description)
{
    TR_DEBUG("Enter");
    return impl_->send_event_periodic(
        data, data_length, data_type, callback, period_millisec, description);
}

} // namespace mbl {
