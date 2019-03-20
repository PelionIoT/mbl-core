/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Event.h"
#include "CloudConnectTrace.h"
#include "CloudConnectTypes.h"

#include <systemd/sd-event.h>

#include <cassert>

#define TRACE_GROUP "ccrb-event"

// using namespace std::chrono;

namespace mbl
{

Event::~Event()
{
    TR_DEBUG_ENTER;
    sd_event_unref(event_loop_handle_);
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
