/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _EventImmediate_h_
#define _EventImmediate_h_

#include "CloudConnectTrace.h"
#include "Event.h"

#include <string>

namespace mbl
{

class EventManager;

/**
 * @brief This class implements an immidiate sent event. It does not support sending events by
 * external
 * threads. Events are sent using EventManager class.
 *
 */
class EventImmediate : public Event
{

public:
    /**
     * @brief send event to the sd event loop
     *
     * @return result - 0 on success, non 0 on error
     */
    virtual int send();

    /**
     * @brief Ctor - Construct a new Immidiate Event object with a template data type T
     *
     * @tparam T - the data type. must be POD type.
     * @param data - the data payload
     * @param data_length - length of actual used data in bytes - can't be more than size(T)
     * @param user_callback - user supplied callback to be called when event fired by event manager
     * @param event_manager_cb - callback to the event manager unmanage_event() method
     * @param description  as std::string
     */
    template <typename T>
    EventImmediate(T& data,
                   unsigned long data_length,
                   UserCallback user_callback,
                   EventManagerCallback event_manager_callback,
                   sd_event* event_loop_handle,
                   const std::string& description)
        : Event(data,
                data_length,
                user_callback,
                event_manager_callback,
                event_loop_handle,
                description)
    {
        mbed_tracef(TRACE_LEVEL_DEBUG, "ccrb-event", "Enter");
    }

private:
    /**
     * @brief static handler function - called for trigerred (fired) events.
     * The *impl function is the actual implementation inside the object
     *
     * @param s - the event source which fired
     * @param userdata - userdata
     * @return int - 0 on success, On failure, return a negative Linux errno-style error code.
     */
    static int immediate_event_handler(sd_event_source* s, void* userdata);

    // Do not create empty events
    EventImmediate() = delete;
    EventImmediate(const EventImmediate&) = delete;
    EventImmediate& operator=(const EventImmediate&) = delete;
    EventImmediate(EventImmediate&&) = delete;
    EventImmediate& operator=(EventImmediate&&) = delete;
};

} // namespace mbl {

#endif // _EventImmediate_h_
