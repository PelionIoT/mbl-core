/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define TRACE_GROUP "ccrb-event"

#include "EventPeriodic.h"
#include "CloudConnectTrace.h"
#include "CloudConnectTypes.h"
#include "MblError.h"

#include <systemd/sd-event.h>

#include <cassert>
#include <chrono>
#include <functional>

using namespace std::chrono;

namespace mbl
{

EventPeriodic::EventPeriodic(EventData& data,
                             unsigned long data_length,
                             EventDataType data_type,
                             UserCallback user_callback,
                             EventManagerCallback event_manager_callback,
                             sd_event* event_loop_handle,
                             uint64_t period_millisec,
                             const std::string& description)
    :

      Event::Event(data,
                   data_length,
                   data_type,
                   user_callback,
                   event_manager_callback,
                   event_loop_handle,
                   description),
      period_millisec_(period_millisec)
{
    TR_DEBUG("Enter");
    assert(period_millisec >= min_periodic_event_duration_millisec &&
           period_millisec <= max_periodic_event_duration_millisec);
}

int EventPeriodic::immediate_event_handler(sd_event_source* s,
                                           uint64_t microseconds,
                                           void* userdata)
{
    TR_DEBUG("Enter");

    assert(userdata);
    assert(s);
    assert(microseconds);
    EventPeriodic* ev = static_cast<EventPeriodic*>(userdata);
    uint64_t when_to_expire_microseconds = 0; // next experation timestamp in microseconds

    // get the timestamp when the most recent event loop iteration began. Clock source
    // CLOCK_MONOTONIC
    // is used.  This clock cannot be set and is not affected by discontinuous jumps in the system
    // time.
    // For more information about different clock sources refer
    // http://man7.org/linux/man-pages/man2/clock_gettime.2.html
    int r = sd_event_now(ev->event_loop_handle_, CLOCK_MONOTONIC, &when_to_expire_microseconds);
    if (r < 0) {
        TR_ERR("sd_event_add_defer failed with error r=%d (%s)", r, strerror(r));
        return r;
    }
    // Event loop iteration has not run yet - this is not an error
    if (r > 0) {
        TR_INFO("sd_event_now: event loop iteration has not run"
                " yet, sd_event_now returned r=%d (%s)",
                r,
                strerror(r));
    }

    TR_DEBUG("sd_event_now: out_event_id=%" PRIu64 " send_time=%" PRIu64
             "sd_event_now() returned when_to_expire=%" PRIu64,
             ev->id_,
             ev->send_time_.count(),
             when_to_expire_microseconds);

    // add period in microseconds
    when_to_expire_microseconds += ev->period_millisec_ * microsec_per_millisec;

    // update experation time
    r = sd_event_source_set_time(ev->sd_event_source_, when_to_expire_microseconds);
    if (r < 0) {
        TR_ERR("sd_event_source_set_time failed with error r=%d (%s) - returning %s",
               r,
               strerror(r),
               MblError_to_str(MblError::DBA_SdEventCallFailure));
        return r;
    }
    // enable the event source
    r = sd_event_source_set_enabled(ev->sd_event_source_, SD_EVENT_ON);
    if (r < 0) {
        TR_ERR("sd_event_source_set_enabled failed with error r=%d (%s) - returning %s",
               r,
               strerror(r),
               MblError_to_str(MblError::DBA_SdEventCallFailure));
        return r;
    }

    TR_DEBUG("sd_event_source_set_enabled, event id %" PRIu64
             " when_to_expire_microseconds = %" PRIu64,
             ev->id_,
             when_to_expire_microseconds);

    // call base class
    ev->on_fire();

    return 0;
}

int EventPeriodic::send()
{
    TR_DEBUG("Enter");

    uint64_t when_to_expire_microseconds = 0; // next experation timestamp in microseconds

    // Send event now. Clock source CLOCK_MONOTONIC is used.  This clock cannot be set and is not
    // affected by discontinuous jumps in the system time. For more information about different
    // clock sources refer http://man7.org/linux/man-pages/man2/clock_gettime.2.html
    int r = sd_event_now(event_loop_handle_, CLOCK_MONOTONIC, &when_to_expire_microseconds);
    if (r < 0) {
        TR_ERR("sd_event_now failed with error r=%d (%s)", r, strerror(r));
        return r;
    }
    // Event loop iteration has not run yet - this is not an error
    if (r > 0) {
        TR_INFO(
            "sd_event_now: event loop iteration has not run yet, sd_event_now returned r=%d (%s)",
            r,
            strerror(r));
    }

    TR_DEBUG("sd_event_now: out_event_id=%" PRIu64 " when_to_expire_microseconds=%" PRIu64,
             id_,
             when_to_expire_microseconds);

    when_to_expire_microseconds += period_millisec_ * microsec_per_millisec;

    // send the timed event, hand the event id_ as userdata (first id is 1)
    // for more details:
    // https://www.freedesktop.org/software/systemd/man/sd_event_add_time.html
    r = sd_event_add_time(event_loop_handle_,
                          &sd_event_source_,
                          CLOCK_MONOTONIC,
                          when_to_expire_microseconds,
                          0, // use default accuracy (250 milliseconds)
                          immediate_event_handler,
                          (void*) this);

    if (r < 0) {
        TR_ERR("sd_event_add_time failed with error r=%d (%s)", r, strerror(r));
        return r;
    }

    // record event send time
    send_time_ = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

    TR_DEBUG("EventPeriodic send: out_event_id=%" PRIu64 " send_time=%" PRIu64
             " when_to_expire_microseconds=%" PRIu64 " data_length=%lu data_type=%s description=%s",
             id_,
             send_time_.count(),
             when_to_expire_microseconds,
             data_length_,
             Event::EventType_to_str(data_type_),
             description_.c_str());

    return 0;
}

} // namespace mbl {
