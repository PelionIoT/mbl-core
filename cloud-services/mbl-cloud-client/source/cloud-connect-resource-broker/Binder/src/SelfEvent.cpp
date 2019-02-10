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

uint64_t SelfEvent::next_event_id_ = 0;


SelfEvent::SelfEvent(EventData data, DataType data_type, const std::string& description, SelfEventCallback callback) :
    id_(next_event_id_++),
    data_type_(data_type), 
    description_(description), 
    data_(data),
    creation_time_(duration_cast< milliseconds >(system_clock::now().time_since_epoch())),
    send_time_(0),
    fire_time_(),
    callback_(callback),
    event_source_handle_(nullptr),
    event_loop_handle_(nullptr)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(callback);
}


SelfEvent::SelfEvent(EventData data, DataType data_type, const char* description, SelfEventCallback callback) :
    SelfEvent::SelfEvent(data, data_type, std::string(description), callback)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(description);    
}

int SelfEvent::on_fire()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    fire_time_ = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    
    int r = sd_event_source_set_enabled(event_source_handle_, SD_EVENT_OFF);
    if (r < 0){
        return r;
    }
    MblError status = callback_(this);
    sd_event_source_unref(event_source_handle_);
    event_source_handle_ = nullptr;
    sd_event_unref(event_loop_handle_);
    event_loop_handle_ = nullptr;
    return (MblError::None == status) ? 0 : -1;
}

static int self_event_handler(sd_event_source *s, void *userdata) 
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(s);
    SelfEvent *ev = static_cast<SelfEvent*>(userdata);
    int r = ev->on_fire();    
}


MblError SelfEvent::send()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    int r;        

    r = sd_event_default(&event_loop_handle_);
    if (r < 0){
        return MblError::DBusErr_Temporary;
    }
    
    send_time_ = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    r = sd_event_add_defer(event_loop_handle_, &event_source_handle_, self_event_handler, this);
    if (r < 0){
        return MblError::DBusErr_Temporary;
    }

    return MblError::None;
}


SelfEvent::~SelfEvent()
{
    sd_event_unref(event_loop_handle_);
}
} // namespace mbl {

