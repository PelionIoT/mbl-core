/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _EventManager_h_
#define _EventManager_h_

#include <inttypes.h>
#include <systemd/sd-event.h>

#include "SelfEvent.h"

namespace mbl {

class EventManager
{
public:
    static MblError send_event_immediate(
        SelfEvent::EventData data, SelfEvent::DataType data_type, 
        const std::string& description, SelfEventCallback callback, uint64_t &out_event_id
    );
private:
    static std::map< sd_event_source *, std::unique_ptr<SelfEvent> >  events_;
    static int self_event_handler(sd_event_source *s, void *userdata);

    static uint64_t next_event_id_;
};

} //  namespace mbl {

#endif //_EventManager_h_