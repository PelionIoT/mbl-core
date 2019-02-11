/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define TRACE_GROUP "ccrb-events"

#include "CloudConnectTrace.h"
#include "SelfEvent.h"
#include "MblError.h"
#include "CloudConnectTypes.h"

#include <systemd/sd-event.h>

#include <chrono>
#include <functional>
#include <cassert>

using namespace std::chrono;

namespace mbl {

SelfEvent::SelfEvent(
    EventManager &event_manager,
    EventData &data,
    unsigned long data_length,
    EventType event_type,    
    SelfEventCallback callback,
    const std::string& description) :
       
    data_(data),
    data_length_(data_length),
    event_type_(event_type), 
    callback_(callback),
    description_(description),    
    id_(0),   // id is assigned by event manager     
    creation_time_(duration_cast< milliseconds > (system_clock::now().time_since_epoch())),
    fire_time_(0),
    send_time_(0),        
    event_manager_(event_manager)       
{
    TR_DEBUG("Enter");
    assert(callback);
    assert(data_length_ <= sizeof(data_));   // don't assert by type, just avoid corruption
}

SelfEvent::SelfEvent(
    EventManager &event_manager,
    EventData &data,
    unsigned long data_length,
    EventType event_type,    
    SelfEventCallback callback,
    const char* description) :
    SelfEvent::SelfEvent(
        event_manager,
        data, 
        data_length,
        event_type, 
        callback, 
        std::string(description))
{
    TR_DEBUG("Enter");
    assert(description);    
}

const char* SelfEvent::get_data_type_str()
{
    return  EventType_to_str(this->event_type_);
}

const char*  SelfEvent::EventType_to_str(EventType type)
{
    switch (type)
    {
        RETURN_STRINGIFIED_VALUE(SelfEvent::EventType::RAW); 

        default:
            return "Unknown Event Type!";
    }
}

} // namespace mbl {
