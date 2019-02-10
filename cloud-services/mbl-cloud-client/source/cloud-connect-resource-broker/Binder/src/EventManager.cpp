/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <chrono>

#include "EventManager.h"

using namespace std::chrono;

namespace mbl {

uint64_t EventManager::next_event_id_ = 0;
std::map< sd_event_source *, std::unique_ptr<SelfEvent> >  EventManager::events_;

int EventManager::self_event_handler(sd_event_source *s, void *userdata)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(s);
    assert(userdata);
    auto iter = events_.find(s);
    if (events_.end() == iter){
        return -1;
    }
    auto &ev = iter->second;    
    ev->fire_time_ = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    
    int r = sd_event_source_set_enabled(ev->event_source_handle_, SD_EVENT_OFF);
    if (r < 0){
        return r;
    }
    MblError status = ev->callback_(ev.get());    
    events_.erase(iter);
    return (MblError::None == status) ? 0 : -1;
}


MblError EventManager::send_event_immediate(
    SelfEvent::EventData data, 
    SelfEvent::DataType data_type, 
    const std::string& description, 
    SelfEventCallback callback, 
    uint64_t &out_event_id)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    std::unique_ptr<SelfEvent> ev(new SelfEvent(data, data_type, description, callback));    
    int r;        

    r = sd_event_default(&ev->event_loop_handle_);
    if (r < 0){
        return MblError::DBusErr_Temporary;
    }
    
    ev->send_time_ = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    r = sd_event_add_defer(
        ev->event_loop_handle_, 
        &ev->event_source_handle_, 
        self_event_handler, 
        (void*)++next_event_id_);
    if (r < 0){
        next_event_id_--;
        return MblError::DBusErr_Temporary;
    }
    ev->id_ = next_event_id_;
    events_[ev->event_source_handle_] = std::move(ev);
    return MblError::None;    
}

} //namespace mbl {