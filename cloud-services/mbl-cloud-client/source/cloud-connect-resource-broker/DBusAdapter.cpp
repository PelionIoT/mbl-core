/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "CloudConnectTrace.h"
#include "ResourceBroker.h"
#include "DBusAdapter.h"
#include "DBusAdapter_Impl.h"

#include <cassert>

#define TRACE_GROUP "ccrb-dbus"

namespace mbl {

/*
For holding std::unique_ptr<DBusAdapterImpl*>, need to allow unique_ptr to recognize DBusAdapterImpl
as a complete type. std::unique_ptr checks  in its destructor if the definition of the type is 
visible before calling delete.
DBusAdapterImpl is forward decleared in header (that a call to delete is well-formed);
std::unique_ptr will refuse to compile and to call delete if the type is only forward declared.
Use the default implentation for the destructor after the definition of DBusAdapterImpl :
*/
DBusAdapter::~DBusAdapter() = default;

DBusAdapter::DBusAdapter(ResourceBroker &ccrb) :
    // initialize impl_ unique_ptr
    impl_(new DBusAdapterImpl(ccrb))
{
    TR_DEBUG("Enter");
}

MblError DBusAdapter::init()
{
    TR_DEBUG("Enter");
    MblError status = impl_->init();
    if (status != MblError::None){
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

MblError DBusAdapter::run(MblError &stop_status)
{
    TR_DEBUG("Enter");
    MblError status = impl_->run(stop_status);
    if (status != MblError::None){
        //best effort - stop
        impl_->stop(status);
    }
    return status;
}

MblError DBusAdapter::stop(MblError stop_status)
{
   TR_DEBUG("Enter");
   return impl_->stop(stop_status); 
}


MblError DBusAdapter::update_registration_status(
        const uintptr_t ipc_request_handle, 
        const CloudConnectStatus reg_status)
{
    TR_DEBUG("Enter");
    return impl_->handle_ccrb_RegisterResources_status_update(ipc_request_handle, reg_status); 
}

MblError DBusAdapter::update_deregistration_status(
    const uintptr_t ipc_request_handle, 
    const CloudConnectStatus dereg_status)
{
    TR_DEBUG("Enter");
    return impl_->handle_ccrb_DeregisterResources_status_update(ipc_request_handle, dereg_status);
}

MblError DBusAdapter::update_add_resource_instance_status(
    const uintptr_t ipc_request_handle, 
    const CloudConnectStatus add_status)
{
    TR_DEBUG("Enter");
    return impl_->handle_ccrb_AddResourceInstances_status_update(ipc_request_handle, add_status);
}

MblError DBusAdapter::update_remove_resource_instance_status(
    const uintptr_t ipc_request_handle, 
    const CloudConnectStatus remove_status)
{    
    TR_DEBUG("Enter");
    return impl_->handle_ccrb_RemoveResourceInstances_status_update(
        ipc_request_handle, remove_status);
}

MblError DBusAdapter::send_event_immediate(
    SelfEvent::EventDataType data,
    unsigned long data_length,
    SelfEvent::EventType data_type,        
    SelfEventCallback callback,
    uint64_t &out_event_id,
    const char *description)
{
    assert(callback);
    TR_DEBUG("Enter");
    return impl_->send_event_immediate(
        data, data_length, data_type, callback, out_event_id, description);
}
    
} // namespace mbl {

