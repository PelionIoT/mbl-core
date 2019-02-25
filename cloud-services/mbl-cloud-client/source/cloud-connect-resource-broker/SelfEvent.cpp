/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define TRACE_GROUP "ccrb-events"

#include "SelfEvent.h"
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

SelfEvent::SelfEvent(EventManager& event_manager,
                     EventData& data,
                     unsigned long data_length,
                     EventDataType data_type,
                     SelfEventCallback callback,
                     const std::string description)
    :

      data_(data),
      data_length_(data_length),
      data_type_(data_type),
      callback_(callback),
      description_(std::move(description)),
      id_(0), // id is assigned by event manager
      creation_time_(duration_cast<milliseconds>(system_clock::now().time_since_epoch())),
      fire_time_(0),
      send_time_(0),
      event_manager_(event_manager)
{
    TR_DEBUG("Enter");
    assert(callback);
    assert(data_length_ <= sizeof(data_)); // don't assert by type, just avoid corruption
}

SelfEvent::SelfEvent(EventManager& event_manager,
                     EventData& data,
                     unsigned long data_length,
                     EventDataType data_type,
                     SelfEventCallback callback,
                     const char* description)
    : SelfEvent::SelfEvent(
          event_manager, data, data_length, data_type, callback, std::string(description))
{
    TR_DEBUG("Enter");
    assert(description);
}

const char* SelfEvent::get_data_type_str()
{
    return DataType_to_str(this->data_type_);
}

const char* SelfEvent::DataType_to_str(EventDataType type)
{
    switch (type)
    {
        RETURN_STRINGIFIED_VALUE(SelfEvent::EventDataType::RAW);

    default: return "Unknown Event Type!";
    }
}

} // namespace mbl {
