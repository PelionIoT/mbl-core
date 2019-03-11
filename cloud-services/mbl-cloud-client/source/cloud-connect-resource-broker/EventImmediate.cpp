/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define TRACE_GROUP "ccrb-event"

#include "EventImmediate.h"
#include "CloudConnectTrace.h"
#include "CloudConnectTypes.h"
#include "EventManager.h"
#include "MblError.h"

#include <systemd/sd-event.h>

#include <cassert>
#include <chrono>
#include <functional>

using namespace std::chrono;

namespace mbl
{

EventImmediate::EventImmediate(EventData& data,
                               unsigned long data_length,
                               EventDataType data_type,
                               UserCallback user_callback,
                               EventManagerCallback event_manager_callback,
                               sd_event* event_loop_handle,
                               const std::string& description)
    : Event(data,
            data_length,
            data_type,
            user_callback,
            event_manager_callback,
            event_loop_handle,
            description)
{
    TR_DEBUG_ENTER;
    assert(data_length_ <= sizeof(data_)); // don't assert by type, just avoid corruption
}

int EventImmediate::immediate_event_handler(sd_event_source* s, void* userdata)
{
    TR_DEBUG_ENTER;

    assert(userdata);
    assert(s);
    EventImmediate* ev = static_cast<EventImmediate*>(userdata);

    // call base class
    ev->on_fire();

    ev->sd_event_source_ = sd_event_source_unref(ev->sd_event_source_);

    return ev->event_manager_callback_(s, ev);
}

int EventImmediate::send()
{
    TR_DEBUG_ENTER;

    // send the event, hand the event id_ as userdata (first id is 1)
    // for more details:
    // https://www.freedesktop.org/software/systemd/man/sd_event_add_defer.html#
    int r = sd_event_add_defer(
        event_loop_handle_, &sd_event_source_, immediate_event_handler, (void*) this);

    if (r < 0) {
        TR_ERR("sd_event_add_defer failed with error r=%d (%s) - returning %s",
               r,
               strerror(r),
               MblError_to_str(MblError::DBA_SdEventCallFailure));
        return r;
    }

    // record event send time
    send_time_ = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

    TR_DEBUG("EventImmediate sent: out_event_id=%" PRIu64 " send_time=%" PRIu64
             " data_length=%lu data_type=%s description=%s",
             id_,
             send_time_.count(),
             data_length_,
             Event::EventType_to_str(data_type_),
             description_.c_str());

    return 0;
}

} // namespace mbl {
