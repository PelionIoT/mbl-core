/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TestInfra_DBusAdapterTester.h"
#include "CloudConnectTrace.h"
#include "DBusAdapter.h"
#include "DBusAdapter_Impl.h"
#include "TestInfra.h"

#include <systemd/sd-event.h>

#include <string>

#define TRACE_GROUP "ccrb-event"

using namespace mbl;

MblError TestInfra_DBusAdapterTester::validate_deinitialized_adapter()
{
    TR_DEBUG_ENTER;
    TESTER_VALIDATE_EQ(adapter_.impl_->state_.get(),
                       mbl::DBusAdapterImpl::DBusAdapterState::UNINITALIZED);
    TESTER_VALIDATE_EQ(adapter_.impl_->event_loop_handle_, nullptr);
    TESTER_VALIDATE_EQ(adapter_.impl_->connection_handle_, nullptr);
    TESTER_VALIDATE_EQ(adapter_.impl_->unique_name_, nullptr);
    TESTER_VALIDATE_EQ(adapter_.impl_->service_name_, nullptr);
    return MblError::None;
}

MblError TestInfra_DBusAdapterTester::event_loop_request_stop(MblError stop_status)
{
    TR_DEBUG_ENTER;
    TESTER_VALIDATE_EQ(adapter_.impl_->event_loop_request_stop(stop_status), MblError::None);
    return MblError::None;
}

MblError TestInfra_DBusAdapterTester::event_loop_run(MblError& stop_status,
                                                     MblError expected_stop_status)
{
    TR_DEBUG_ENTER;
    TESTER_VALIDATE_EQ(adapter_.impl_->event_loop_run(stop_status), MblError::None);
    TESTER_VALIDATE_EQ(stop_status, expected_stop_status);
    return MblError::None;
}

sd_event* TestInfra_DBusAdapterTester::get_event_loop_handle()
{
    return adapter_.impl_->event_loop_handle_;
}

int TestInfra_DBusAdapterTester::send_event_defer(sd_event_handler_t handler, void* userdata)
{
    TR_DEBUG_ENTER;
    return sd_event_add_defer(adapter_.impl_->event_loop_handle_, nullptr, handler, userdata);
}

void TestInfra_DBusAdapterTester::unref_event_source(Event* ev)
{
    TR_DEBUG_ENTER;
    ev->sd_event_source_ = sd_event_source_unref(ev->sd_event_source_);
}

const Event::EventManagerCallback
TestInfra_DBusAdapterTester::get_event_manager_callback(Event* ev) const
{
    return ev->event_manager_callback_;
}
