/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "EventManager.h"
#include "CloudConnectTrace.h"
#include "CloudConnectTypes.h"

#include <cassert>

#define TRACE_GROUP "ccrb-events"

using namespace std::chrono;

namespace mbl
{

EventManager::EventManager() : next_event_id_(1), event_loop_handle_(nullptr) { TR_DEBUG("Enter"); }

EventManager::~EventManager() { TR_DEBUG("Enter"); }

MblError EventManager::init()
{
    // get a reference (or create a new one) to the default sd-event loop
    int r = sd_event_default(&event_loop_handle_);
    if (r < 0) {
        TR_ERR("sd_event_default failed with error r=%d (%s) - returning %s", r, strerror(r),
               MblError_to_str(MblError::DBA_SdEventCallFailure));
        return MblError::DBA_SdEventCallFailure;
    }
    assert(event_loop_handle_);

    return MblError::None;
}

MblError EventManager::deinit()
{
    TR_DEBUG("Enter");

    // dereference event loop handle
    sd_event_unref(event_loop_handle_);
    event_loop_handle_ = nullptr;

    // The actual event object are destroyed by the unique pointers and the event sources are
    // dereferenced in event dtor
    return MblError::None;
}

int EventManager::self_event_handler_impl(sd_event_source* s, SelfEvent* ev)
{
    TR_DEBUG("Enter");
    assert(ev);
    assert(s);

    // look for the event id in map - this is done in order to validate it's existance
    auto iter = events_.find(ev->id_);
    if (events_.end() == iter) {
        TR_ERR("event id_=%" PRIu64 " not found in events_!", ev->id_);
        return (-EINVAL);
    }

    // record fire time
    ev->fire_time_ = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

    // trigger the user callback - callback returned value is ignored and used only for debugging
    MblError status = ev->callback_(s, ev);
    // TODO - remove next line
    UNUSED(status);

    TR_DEBUG("=SelfEvent fired= (callback status=%s) : id_=%" PRIu64 " send_time=%" PRIu64
             " fire_time=%" PRIu64 " data_length=%lu data_type=%s description=%s",
             MblError_to_str(status), ev->id_, ev->send_time_.count(), ev->fire_time_.count(),
             ev->data_length_, ev->get_data_type_str(), ev->description_.c_str());

    // callback is done - remove the event  from map
    // TODO - this is done only for immidiate/delayed events. Later on need to implement different
    // behavior for periodic events
    // TODO -IOTMBL-1686 consider adding a "free pool" to avoid dynamic allocations and
    // deallocations, this will be a vector or queue of pre allocated elements, with a bitmap
    // pointing. on free elements. if no empty left - double the size. need 2 functions with lock
    // guard at the entrance - something like get_free_element and return_element
    //(if the pool is empty, we may use dynamic allocation) this pool will hold references.
    events_.erase(iter);

    return 0;
}

int EventManager::self_event_handler(sd_event_source* s, void* userdata)
{
    UNUSED(s);
    assert(userdata);
    assert(s);
    SelfEvent* ev = static_cast<SelfEvent*>(userdata);
    return ev->event_manager_.self_event_handler_impl(s, ev);
}

MblError EventManager::send_event_immediate(SelfEvent::EventData data, unsigned long data_length,
                                            SelfEvent::EventDataType data_type,
                                            SelfEventCallback callback, uint64_t& out_event_id,
                                            const std::string& description)
{
    TR_DEBUG("Enter");
    switch (data_type)
    {
    case SelfEvent::EventDataType::RAW:
        if (data_length > SelfEvent::EventData::MAX_BYTES) {
            TR_ERR("Illegal data_length of size %lu > %d", data_length,
                   SelfEvent::EventData::MAX_BYTES);
            return MblError::DBA_InvalidValue;
        }
        break;

    default:
        TR_ERR("Invalid data_type!");
        return MblError::DBA_InvalidValue;
    }

    // create the event
    std::unique_ptr<SelfEvent> ev(
        new SelfEvent(*this, data, data_length, data_type, callback, description));

    // record event send time
    ev->send_time_ = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    std::chrono::milliseconds send_time = ev->send_time_;

    // assign event id
    ev->id_ = next_event_id_;

    // send the event, hand the next_event_id_ as userdata (first id is 1)
    // for more details:
    // https://www.freedesktop.org/software/systemd/man/sd_event_add_defer.html#
    int r = sd_event_add_defer(event_loop_handle_, nullptr, self_event_handler, (void*) ev.get());
    if (r < 0) {
        TR_ERR("sd_event_add_defer failed with error r=%d (%s) - returning %s", r, strerror(r),
               MblError_to_str(MblError::DBA_SdEventCallFailure));
        return MblError::DBA_SdEventCallFailure;
    }

    // increment event id , fill for user and store to map
    out_event_id = next_event_id_;
    next_event_id_++;
    events_[out_event_id] = std::move(ev);

    TR_DEBUG("=SelfEvent sent== out_event_id=%" PRIu64 " send_time=%" PRIu64
             " data_length=%lu data_type=%s description=%s",
             out_event_id, send_time.count(), data_length, SelfEvent::DataType_to_str(data_type),
             description.c_str());

    return MblError::None;
}

MblError EventManager::send_event_immediate(SelfEvent::EventData data, unsigned long data_length,
                                            SelfEvent::EventDataType data_type,
                                            SelfEventCallback callback, uint64_t& out_event_id,
                                            const char* description)
{
    return send_event_immediate(data, data_length, data_type, callback, out_event_id,
                                std::string(description));
}

} // namespace mbl {
