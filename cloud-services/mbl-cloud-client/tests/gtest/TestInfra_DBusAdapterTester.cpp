/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "DBusAdapter.h"
#include "DBusAdapter_Impl.h"
#include "TestInfra_DBusAdapterTester.h"
#include "CloudConnectTrace.h"
#include "TestInfra.h"

#define TRACE_GROUP "dbus-gtest-infra"

using namespace mbl;

MblError TestInfra_DBusAdapterTester::validate_deinitialized_adapter()
{
    TR_DEBUG("Enter");
    TESTER_VALIDATE_EQ(
        adapter_.impl_->state_.get(), 
        mbl::DBusAdapterImpl::DBusAdapterState::UNINITALIZED);
    TESTER_VALIDATE_EQ(adapter_.impl_->pending_messages_.empty(), true);
    TESTER_VALIDATE_EQ(adapter_.impl_->event_loop_handle_, nullptr);
    TESTER_VALIDATE_EQ(adapter_.impl_->connection_handle_, nullptr);
    TESTER_VALIDATE_EQ(adapter_.impl_->unique_name_, nullptr);
    TESTER_VALIDATE_EQ(adapter_.impl_->service_name_, nullptr);
    return MblError::None;
}

MblError TestInfra_DBusAdapterTester::event_loop_request_stop(MblError stop_status)
{
    TR_DEBUG("Enter");
    TESTER_VALIDATE_EQ(adapter_.impl_->event_loop_request_stop(stop_status), MblError::None);
    return MblError::None;
}

MblError TestInfra_DBusAdapterTester::event_loop_run(
    MblError &stop_status, MblError expected_stop_status)
{
    TR_DEBUG("Enter");
    TESTER_VALIDATE_EQ(adapter_.impl_->event_loop_run(stop_status), MblError::None);
    TESTER_VALIDATE_EQ(stop_status, expected_stop_status);
    return MblError::None;
}

sd_event *TestInfra_DBusAdapterTester::get_event_loop_handle()
{
    return adapter_.impl_->event_loop_handle_;
}

int TestInfra_DBusAdapterTester::send_event_defer(
    sd_event_handler_t handler,
    void *userdata)
{
    TR_DEBUG("Enter");
    return sd_event_add_defer(
        adapter_.impl_->event_loop_handle_,
        nullptr,
        handler,
        userdata);
}

MblError TestInfra_DBusAdapterTester::send_event_immediate(
        SelfEvent::EventData data,
        unsigned long data_length,
        SelfEvent::EventDataType data_type,        
        SelfEventCallback callback,
        uint64_t &out_event_id,
        const char* description)
{
    return adapter_.impl_->event_manager_.send_event_immediate(
        data,
        data_length,
        data_type,
        callback,
        out_event_id,
        description);    
}