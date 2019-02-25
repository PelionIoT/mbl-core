/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _EventPeriodic_h_
#define _EventPeriodic_h_

#include "Event.h"
#include "MblError.h"

#include <string>

namespace mbl
{

class EventManager;

/**
 * @brief This class implements a self sent periodic event. It does not support sending events by
 * external threads. Events are sent using EventManager class.
 *
 */
class EventPeriodic : public Event
{
public:
    static const uint32_t millisec_per_sec = 1000;
    static const uint32_t microsec_per_millisec = 1000;
    // minimal period in milliseconds for timed events
    static const uint32_t min_periodic_event_duration_millisec = 100;
    // maximal period in milliseconds for timed events is 10 days
    static constexpr uint64_t max_periodic_event_duration_millisec =
        millisec_per_sec * 60 * 60 * 24 * 10;

    /**
     * @brief Public ctor - Construct a new Periodic Event object.
     *
     * @param data - the data payload
     * @param data_length - length of actual used data in bytes - can't be more than maximum allowed
     * by the matching type in EventData
     * @param data_type the data type
     * @param user_callback - user supplied callback to be called when event fired by event manager
     * @param event_manager_cb - callback to the event manager unmanage_event() method
     * @param description - description as std::string
     */
    EventPeriodic(EventData& data,
                  unsigned long data_length,
                  EventDataType data_type,
                  UserCallback user_callback,
                  EventManagerCallback event_manager_callback,
                  sd_event* event_loop_handle,
                  uint64_t period_millisec,
                  const std::string& description);

    // Getters - inline implemented
    inline uint64_t get_period_millisec() const { return period_millisec_; }

    /**
     * @brief send timed event to the sd event loop
     *
     * @return result - 0 on success, non 0 on error
     */
    virtual int send();

protected:
    uint64_t period_millisec_;
    sd_event_source* event_source_;

private:
    /**
     * @brief static handler function - called for trigerred (fired) time events.
     * The *impl function is the actual implementation inside the object
     *
     * @param s - the event source which fired
     * @param microseconds - specifies the earliest time, in microseconds (µs), relative to the
     * clock's epoch, when the timer shall be triggered
     * @param userdata - userdata
     * @return int - 0 on success, On failure, return a negative Linux errno-style error code.
     */
    static int immediate_event_handler(sd_event_source* s, uint64_t microseconds, void* userdata);

    // Do not create empty events
    EventPeriodic() = delete;
    EventPeriodic(const EventPeriodic&) = delete;
    EventPeriodic& operator=(const EventPeriodic&) = delete;
    EventPeriodic(EventPeriodic&&) = delete;
    EventPeriodic& operator=(EventPeriodic&&) = delete;
};

} // namespace mbl {

#endif // _EventPeriodic_h_
