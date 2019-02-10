/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef _DBusAdapterTimer_h_
#define _DBusAdapterTimer_h_

#include <string>
#include <functional>
#include <chrono>

#include <systemd/sd-event.h>

#include "MblError.h"

namespace mbl {

typedef std::function<MblError(const class SelfEvent*)> SelfEventCallback;


class SelfEvent
{
public:
    static const int MAX_SIZE_EVENT_DATA_RAW  = 100;    

    typedef union DataType_ {
        //use this struct when data_type == DataType::RAW
        struct EventData_Raw {
            uint8_t     bytes[MAX_SIZE_EVENT_DATA_RAW];
        } raw;
    }EventData;

    enum class DataType
    {
        RAW = 1,
    };

    SelfEvent(EventData data, DataType data_type, const std::string& description, SelfEventCallback callback);
    SelfEvent(EventData data, DataType data_type, const char* description, SelfEventCallback callback);
    ~SelfEvent();

    MblError                    send();
    int                         on_fire();

    const EventData&            get_data() const { return data_; }
    uint64_t                    get_id() const{ return id_; }
    DataType                    get_data_type() const{ return data_type_; };
    const char*                 get_description() const{ return description_.c_str(); }
    const std::chrono::milliseconds   get_creation_time() const{ return creation_time_; }
    std::chrono::milliseconds   get_send_time() const{ return send_time_; }
    std::chrono::milliseconds   get_fire_time() const{ return fire_time_; }
    sd_event_source *           get_event_source() const{ return event_source_handle_ ; }
    sd_event *                  get_event_loop_handle() const{ return event_loop_handle_ ; }

protected:
    uint64_t                        id_;
    DataType                        data_type_;
    std::string                     description_;
    EventData                       data_;
    const std::chrono::milliseconds creation_time_;
    std::chrono::milliseconds       fire_time_;
    std::chrono::milliseconds       send_time_;
    SelfEventCallback               callback_;
    sd_event_source *               event_source_handle_;
    sd_event *                      event_loop_handle_;

private:
    static uint64_t     next_event_id_;

    SelfEvent() = delete;
    SelfEvent(const SelfEvent&) = delete;
    SelfEvent & operator = (const SelfEvent&) = delete;
    SelfEvent(SelfEvent&&) = delete;
    SelfEvent& operator = (SelfEvent&&) = delete; 
};

//class SelfEventDelayed : public SelfEvent
//{

//}


} // namespace mbl {

#endif // _DBusAdapterTimer_h_
