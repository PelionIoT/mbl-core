/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "EventManager.h"
#include "CloudConnectTrace.h"
#include "CloudConnectTypes.h"
#include "EventImmediate.h"
#include "EventPeriodic.h"

#include <systemd/sd-event.h>

#include <cassert>
#include <functional>

#define TRACE_GROUP "ccrb-event"

using namespace std::chrono;

namespace mbl
{

EventManager::EventManager() : event_loop_handle_(nullptr)
{
    TR_DEBUG_ENTER;
}

MblError EventManager::init()
{
    TR_DEBUG_ENTER;

    // get a reference (or create a new one) to the default sd-event loop
    int r = sd_event_default(&event_loop_handle_);
    if (r < 0) {
        TR_ERR("sd_event_default failed with error r=%d (%s) - returning %s",
               r,
               strerror(-r),
               MblError_to_str(MblError::DBA_SdEventCallFailure));
        return MblError::DBA_SdEventCallFailure;
    }
    assert(event_loop_handle_);

    return MblError::None;
}

MblError EventManager::deinit()
{
    TR_DEBUG_ENTER;

    // in order to free an event loop object, all remaining event sources of the event loop
    // need to be freed as each keeps a reference to it.
    for (const auto& p : events_) {
        sd_event_source_unref(p.second->get_sd_event_source());
    }

    // dereference event loop handle
    sd_event_unref(event_loop_handle_);
    event_loop_handle_ = nullptr;

    // The actual event object are destroyed by the unique pointers and the event sources are
    // dereferenced in event dtor
    return MblError::None;
}

int EventManager::unmanage_event(sd_event_source* s, Event* ev)
{
    TR_DEBUG_ENTER;
    assert(ev);
    assert(s);

    // look for the event id in map - this is done in order to validate it's existance
    auto iter = events_.find(ev->get_id());
    if (events_.end() == iter) {
        TR_ERR("event id_=%" PRIu64 " not found in events_!", ev->get_id());
        return (-EINVAL);
    }

    // TODO:IOTMBL-1686 consider adding a "free pool" to avoid dynamic allocations and
    // deallocations, this will be a vector or queue of pre allocated elements, with a bitmap
    // pointing. on free elements. if no empty left - double the size. need 2 functions with lock
    // guard at the entrance - something like get_free_element and return_element
    events_.erase(iter);

    return 0;
}

void EventManager::do_send_event(std::unique_ptr<Event>& ev)
{
    TR_DEBUG_ENTER;
    assert(ev);

    // send immediate event
    ev->send();

    // move event into the map of events
    events_[ev->get_id()] = std::move(ev);
}

std::pair<MblError, uint64_t> EventManager::send_event_immediate(Event::EventData data,
                                                                 unsigned long data_length,
                                                                 Event::EventDataType data_type,
                                                                 Event::UserCallback callback,
                                                                 const std::string& description)
{
    TR_DEBUG_ENTER;

    if (!validate_common_event_parameters(data_type, data_length)) {
        return std::make_pair(MblError::DBA_InvalidValue, UINTMAX_MAX);
    }
    // create the event
    std::unique_ptr<Event> ev(new EventImmediate(
        data,
        data_length,
        data_type,
        callback,
        std::bind(
            &EventManager::unmanage_event, this, std::placeholders::_1, std::placeholders::_2),
        event_loop_handle_,
        description));

    uint64_t id = ev->get_id();

    do_send_event(ev);

    return std::make_pair(MblError::None, id);
}

std::pair<MblError, uint64_t> EventManager::send_event_periodic(Event::EventData data,
                                                                unsigned long data_length,
                                                                Event::EventDataType data_type,
                                                                Event::UserCallback callback,
                                                                uint64_t period_millisec,
                                                                const std::string& description)
{
    TR_DEBUG_ENTER;

    if (!validate_periodic_event_parameters(data_type, data_length, period_millisec)) {
        return std::make_pair(MblError::DBA_InvalidValue, UINTMAX_MAX);
    }

    // create the event
    std::unique_ptr<Event> ev(new EventPeriodic(
        data,
        data_length,
        data_type,
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

bool EventManager::validate_common_event_parameters(Event::EventDataType data_type,
                                                    unsigned long data_length)
{
    switch (data_type)
    {
    case Event::EventDataType::RAW:
        if (data_length > Event::EventData::MAX_BYTES) {
            TR_ERR(
                "Illegal data_length of size %lu > %d", data_length, Event::EventData::MAX_BYTES);
            return false;
        }
        break;
    default: TR_ERR("Invalid data_type!"); return false;
    }

    return true;
}

bool EventManager::validate_periodic_event_parameters(Event::EventDataType data_type,
                                                      unsigned long data_length,
                                                      uint64_t period_millisec)
{
    if (period_millisec < EventPeriodic::min_periodic_event_duration_millisec ||
        period_millisec > EventPeriodic::max_periodic_event_duration_millisec)
    {
        TR_ERR("Illegal period_millisec  %" PRIu64 " , minimum is %lu milliseconds,"
               "maximum is %lu milliseconds",
               period_millisec,
               (unsigned long) EventPeriodic::min_periodic_event_duration_millisec,
               (unsigned long) EventPeriodic::max_periodic_event_duration_millisec);
        return false;
    }

    return validate_common_event_parameters(data_type, data_length);
}

} // namespace mbl {
