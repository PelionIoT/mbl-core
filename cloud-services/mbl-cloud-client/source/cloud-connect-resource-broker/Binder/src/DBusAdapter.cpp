/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cassert>

//TODO - add this in all new production files - should i add in tests too?
//#include "mbed-trace/mbed_trace.h"  // FIXME : uncomment

#include "DBusAdapter.h"
#include "DBusAdapter_Impl.h"

//TODO include also upper layer header

#define TRACE_GROUP "ccrb-dbus"  //TODO - add to all new production files

namespace mbl {

/*
For holding std::unique_ptr<DBusAdapterImpl*>, need to allow unique_ptr to recognize DBusAdapterImpl as a complete type.
std::unique_ptr checks  in its destructor if the definition of the type is visible before calling delete.
DBusAdapterImpl is forward decleared in header (that a call to delete is well-formed);
std::unique_ptr will refuse to compile and to call delete if the type is only forward declared.
Use the default implentation for the destructor after the definition of DBusAdapterImpl :
*/
DBusAdapter::~DBusAdapter() = default;


//TODO  delete - temporary ctor
DBusAdapter::DBusAdapter() : impl_(new DBusAdapterImpl)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

//TODO  uncomment and solve gtest issue for unit tests
/*
DBusAdapter::DBusAdapter(ResourceBroker &ccrb) , mpl_(new DBusAdapterImpl):
    ccrb_(ccrb)
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    // set callbacks to DBUS lower layer    
    lower_level_callbacks_.register_resources_async_callback = register_resources_async_callback;
    lower_level_callbacks_.deregister_resources_async_callback = deregister_resources_async_callback;
}
*/

MblError DBusAdapter::init()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    MblError status = impl_->init();
    if (status != MblError::None){
        impl_->deinit();
    }
    return status;
}

MblError DBusAdapter::deinit()
{
   tr_debug("%s", __PRETTY_FUNCTION__);
   return impl_->deinit(); 
}

MblError DBusAdapter::run(MblError &stop_status)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    MblError status = impl_->run(stop_status);
    if (status != MblError::None){
        impl_->stop(MblError::DBusStopStatusErrorInternal);
    }
    return status;
}

MblError DBusAdapter::stop(MblError stop_status)
{
   tr_debug("%s", __PRETTY_FUNCTION__);
   return impl_->stop(stop_status); 
}


MblError DBusAdapter::update_registration_status(
    const uintptr_t bus_request_handle, 
    const std::string &access_token,
    const CloudConnectStatus status)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    return impl_->handle_ccrb_RegisterResources_status_update(bus_request_handle, access_token, status); 
}

MblError DBusAdapter::update_deregistration_status(
    const uintptr_t bus_request_handle, 
    const CloudConnectStatus status)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    return impl_->handle_ccrb_DeregisterResources_status_update(bus_request_handle, status);
}

MblError DBusAdapter::update_add_resource_instance_status(
    const uintptr_t bus_request_handle, 
    const CloudConnectStatus status)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    return impl_->handle_ccrb_AddResourceInstances_status_update(bus_request_handle, status);
}

MblError DBusAdapter::update_remove_resource_instance_status(
    const uintptr_t bus_request_handle, 
    const CloudConnectStatus status)
{    
    tr_debug("%s", __PRETTY_FUNCTION__);
    return impl_->handle_ccrb_RemoveResourceInstances_status_update(bus_request_handle, status);
}

} // namespace mbl {

