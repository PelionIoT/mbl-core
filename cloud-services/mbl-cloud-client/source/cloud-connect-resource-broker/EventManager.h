/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _EventManager_h_
#define _EventManager_h_

#include "CloudConnectTrace.h"
#include "Event.h"
#include "EventImmediate.h"
#include "EventPeriodic.h"

#include <inttypes.h>

#include <map>
#include <memory>

namespace mbl
{

// TODO: extend this class to support delayed events and delayed periodic events
// TODO: consider adding memory pool to avoid dynamic construction of events
/**
 * @brief The events manager is a utility class which allows the upper layer to easily send itself
 * events. It generates event id for debugging and to allow caller to cancel events.
 * events are kept in an std::map events_ as unique_ptr. It is not thread safe and external threads
 * should not use it at this stage.
 */
class EventManager
{
public:
    EventManager();

    /**
     * @brief initialize the event manager - get reference to event loop
     *
     * @return MblError - Error::None for success running the loop, otherwise the failure reason
     */
    MblError init();

    /**
     * @brief initialize the event manager - release reference on event loop
     *
     * @return MblError - Error::None for success running the loop, otherwise the failure reason
     */
    MblError deinit();

    /**
     * @brief Send an immidiate event with template data T
     * Must be called by CCRB thread only. Send 'defered event' to the event loop using
     * sd_event_add_defer().
     * For more details: https://www.freedesktop.org/software/systemd/man/sd_event_add_defer.html#
     * @param data - the template data to be sent must be POD type
     * @param data_length - length of data used, can't be more than maximum size allowed size(T)
     * @param callback - callback to be called when event is fired
     * @param description - optional - description for the event cause, can leave empty string
     * @return pair - Error::None and generated event id for success, otherwise the failure reason
     * and UINTMAX_MAX
     */
    template <typename T>
    std::pair<MblError, uint64_t> send_event_immediate(T& data,
                                                       unsigned long data_length,
                                                       Event::UserCallback callback,
                                                       const std::string& description = "")
    {
        mbed_tracef(TRACE_LEVEL_DEBUG, "ccrb-event", "Enter");

        // create the event
        std::unique_ptr<Event> ev(new EventImmediate(
            data,
            data_length,
            callback,
            std::bind(
                &EventManager::unmanage_event, this, std::placeholders::_1, std::placeholders::_2),
            event_loop_handle_,
            description));

        uint64_t id = ev->get_id();

        do_send_event(ev);

        return std::make_pair(MblError::None, id);
    }

    /**
    * @brief send periodic event with template data T
    * Must be called by CCRB thread only. Send 'time event' to the event loop using
    * sd_event_add_time().
    * For more details: https://www.freedesktop.org/software/systemd/man/sd_event_add_time.html
    *
    * @tparam T - the type of the event data
    * @param data - the template data to be sent must be POD type
    * @param data_length - length of data used, can't be more than the maximum size allowed size(T)
    * @param callback - callback to be called when event is fired
    * @param period_millisec  - specifies the earliest time, in miliseconds, relative to the
    * clock's epoch, when the timer shall be triggered and event shall be sent. The event will
    * continue to be sent periodically after each period time.
    * @param description - optional - description for the event cause, can leave empty string
    * @return std::pair<MblError, uint64_t> - Error::None and generated event id for success,
    * otherwise the failure reason and UINTMAX_MAX
    */
    template <typename T>
    std::pair<MblError, uint64_t> send_event_periodic(T& data,
                                                      unsigned long data_length,
                                                      Event::UserCallback callback,
                                                      uint64_t period_millisec,
                                                      const std::string& description)
    {
        mbed_tracef(TRACE_LEVEL_DEBUG, "ccrb-event", "Enter");

        if (!validate_periodic_event_parameters(period_millisec)) {
            return std::make_pair(MblError::DBA_InvalidValue, UINTMAX_MAX);
        }

        // create the event
        std::unique_ptr<Event> ev(new EventPeriodic(
            data,
            data_length,
            callback,
            std::bind(
                &EventManager::unmanage_event, this, std::placeholders::_1, std::placeholders::_2),
            event_loop_handle_,
            period_millisec,
            description));

        uint64_t id = ev->get_id();

        do_send_event(ev);

        return std::make_pair(MblError::None, id);
    }

    /**
     * @brief called by immediate_event_handler() for non periodic immediate event after on_fire()
     *
     * @param s - the event source which fired
     * @param ev - the arriving event
     * @return int - 0 for success, else a Linux C style error code.
     */
    int unmanage_event(sd_event_source* s, Event* ev);

private:
    /**
     * @brief called by send_event_immediate()/send_event_periodic().
     * Implements common handling for all types of events.
     *
     * @param ev - the arriving event
     * @return void
     */
    void do_send_event(std::unique_ptr<Event>& ev);

    /**
     * @brief called by send_event_immediate().
     * Checks parameters validity.
     * @param data_type - the type of data to be sent.
     * @param data_length - length of data used, must be less than the maximum size allowed
     * @return bool - true if parameters are valid, else false
     */
    // bool validate_common_event_parameters(EventDataType data_type,
    //                                     unsigned long data_length);

    /**
     * @brief called by send_event_periodic().
     * Checks parameters validity.
     * @param data_type - the type of data to be sent.
     * @param data_length - length of data used, must be less than the maximum size allowed
     * @param period_millisec  - specifies the earliest time, in miliseconds, relative to the
     * @return bool - true if parameters are valid, else false
     */
    bool validate_periodic_event_parameters(uint64_t period_millisec);

    // This map holds all events sent, it holds a pair of (event id, unique_ptr to event)
    std::map<uint64_t, std::unique_ptr<Event>> events_;

    // a handle to the default event loop of CCRB thread
    sd_event* event_loop_handle_;
};

} //  namespace mbl {

#endif //_EventManager_h_
