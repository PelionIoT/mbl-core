/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DBusAdapter_hpp_
#define DBusAdapter_hpp_

#ifndef DBusAdapter_h_
#error "DBusAdapter.h must be included!"
#endif

#include "CloudConnectExternalTypes.h"
#include "Event.h"
#include "MblError.h"
#include "DBusAdapter_Impl.h"

#include <inttypes.h>
#include <memory>
#include <string>

namespace mbl
{

template <typename T>
std::pair<MblError, uint64_t> DBusAdapter::send_event_immediate(T & data,
                                                    unsigned long data_length,
                                                    Event::UserCallback callback,
                                                    const std::string& description)
{
    mbed_tracef(TRACE_LEVEL_DEBUG, "ccrb-event", "Enter");

    return impl_->send_event_immediate(data, data_length, callback, description);
}


template <typename T>
std::pair<MblError, uint64_t> DBusAdapter::send_event_periodic(T& data,
                                                    unsigned long data_length,
                                                    Event::UserCallback callback,
                                                    uint64_t period_millisec,
                                                    const std::string& description)
{
    mbed_tracef(TRACE_LEVEL_DEBUG, "ccrb-event", "Enter");

    return impl_->send_event_periodic(
        data, data_length, callback, period_millisec, description);
}

} // namespace mbl

#endif // #define DBusAdapter_hpp_
