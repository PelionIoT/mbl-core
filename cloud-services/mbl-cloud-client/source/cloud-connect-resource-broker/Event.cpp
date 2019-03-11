/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define TRACE_GROUP "ccrb-event"

#include "Event.h"
#include "CloudConnectTrace.h"
#include "CloudConnectTypes.h"

#include <systemd/sd-event.h>

#include <cassert>

using namespace std::chrono;

namespace mbl
{

Event::Event(EventData& data,
             unsigned long data_length,
             EventDataType data_type,
             UserCallback user_callback,
             EventManagerCallback event_manager_callback,
             sd_event* event_loop_handle,
             std::string description)
    :

      data_(data),
      data_length_(data_length),
      data_type_(data_type),
      user_callback_(user_callback),
      event_manager_callback_(std::move(event_manager_callback)),
      description_(std::string(description)),
      creation_time_(duration_cast<milliseconds>(system_clock::now().time_since_epoch())),
      fire_time_(0),
      send_time_(0),
      sd_event_source_(nullptr),
      event_loop_handle_(event_loop_handle)
{
    TR_DEBUG_ENTER;
    assert(user_callback);
    assert(data_length_ <= sizeof(data_)); // don't assert by type, just avoid corruption
    assert(event_loop_handle_);

    static uint64_t id = 1;
    id_ = id++;

    // get event_loop_handle - this is done in order to get next experation time of the event loop
    sd_event_ref(event_loop_handle_);
}

Event::~Event()
{
    TR_DEBUG_ENTER;
    sd_event_unref(event_loop_handle_);
}

const char* Event::get_data_type_str()
{
    return EventType_to_str(this->data_type_);
}

const char* Event::EventType_to_str(EventDataType type)
{
    switch (type)
    {
        SWITCH_RETURN_STRINGIFIED_VALUE(Event::EventDataType::RAW);

    default: return "Unknown Event Type!";
    }
}

void Event::on_fire()
{
    TR_DEBUG_ENTER;

    // record fire time
    fire_time_ = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

    TR_DEBUG("Event on_fire: event id %" PRIu64 " fire time %" PRIu64, id_, fire_time_.count());

    // trigger the user callback - callback returned value is ignored and used only for debugging
    MblError status = user_callback_(sd_event_source_, this);
    if (status != MblError::None) {
        TR_ERR("user_callback_ failed with error %s", MblError_to_str(status));
        // if user provided callback fails, the failure is not handled here, but only logged.
        // This way the event infrastructure is decoupled from the user provided functionality.
    }
}

} // namespace mbl {
