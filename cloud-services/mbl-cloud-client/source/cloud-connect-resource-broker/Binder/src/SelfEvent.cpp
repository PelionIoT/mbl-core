/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chrono>
#include <functional>
#include <assert.h>

#include <systemd/sd-event.h>

#include "SelfEvent.h"
#include "MblError.h"

using namespace std::chrono;

namespace mbl {

SelfEvent::SelfEvent(EventData &data, DataType data_type, const std::string& description, SelfEventCallback callback) :
    id_(0),     // event with id 0 is not sent yet
    data_type_(data_type), 
    description_(description), 
    data_(data),
    creation_time_(duration_cast< milliseconds > (system_clock::now().time_since_epoch())),
    send_time_(0),
    fire_time_(),
    callback_(callback),
    event_source_handle_(nullptr),
    event_loop_handle_(nullptr)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(callback);
}


SelfEvent::SelfEvent(EventData &data, DataType data_type, const char* description, SelfEventCallback callback) :
    SelfEvent::SelfEvent(data, data_type, std::string(description), callback)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(description);    
}

SelfEvent::~SelfEvent()
{
    sd_event_unref(event_loop_handle_);
    sd_event_source_unref(event_source_handle_);
}

} // namespace mbl {
