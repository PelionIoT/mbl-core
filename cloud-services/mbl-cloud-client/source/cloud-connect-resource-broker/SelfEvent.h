/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SelfEvent_h_
#define _SelfEvent_h_

#include "MblError.h"

#include <systemd/sd-event.h>

#include <string>
#include <functional>
#include <chrono>
#include <memory>
#include <map>

namespace mbl {

#define MAX_SIZE_EVENT_DATA_RAW  100

//never use the SelfEvent or anything inside it after callback is done
typedef std::function<MblError(sd_event_source *s, const class SelfEvent*)> SelfEventCallback;

class EventManager;

/**
 * @brief This class implements a self sent event. It does not support sending events by external 
 * threads. Events are sent using EventManager class (friend class).
 * Currently, only one-time-immidiate event is supported. use inheritance to support more types 
 * of events.
 * 
 */
class SelfEvent
{    
friend class EventManager;
public:

    /**
     * @brief  The event data is a union with structs that define all possible
     * event formats (use only Plain Old data Types).
     * To support variable size - use std containers. if maximal data size is known - developer may
     * use plain old data types.
     * 
     */
    typedef union EventData_ {
        //use this struct when data_type == EventType::RAW
        static const int MAX_BYTES = 100;
        struct EventData_Raw {
            char     bytes[MAX_BYTES];
        } raw;
    }EventDataType;

    enum class EventType
    {
        RAW = 1,
    };
    
    // Getters - inline implemented
    const EventDataType&                get_data() const { return data_; }
    uint64_t                        get_id() const{ return id_; }
    EventType                       get_data_type() const{ return data_type_; };    
    const char*                     get_description() const{ return description_.c_str(); }    
	const std::chrono::milliseconds get_creation_time() const{ return creation_time_; }
    std::chrono::milliseconds       get_send_time() const{ return send_time_; }
    std::chrono::milliseconds       get_fire_time() const{ return fire_time_; }

    // Getters - implemented in cpp
    static const char*              EventType_to_str(EventType type);
    const char*                     get_data_type_str();

protected:
    //event data, may be empty
    EventDataType                   data_;

    // length in bytes of data_
    unsigned long                   data_length_;

    // the event type
    EventType                       data_type_;

    //user callback
    SelfEventCallback               callback_;

    //user may supply both string type which are kept as std:string
    std::string                     description_;

    // The id is automatically assigned
    uint64_t                        id_;

    // creation, send and fire times are recorded
    const std::chrono::milliseconds creation_time_;
    std::chrono::milliseconds       fire_time_;
    std::chrono::milliseconds       send_time_;
    
    //reference to the event manager
    EventManager& event_manager_;
private:

    /**
     * @brief Private ctor - Construct a new Self Event object - 
     * only EventManager can create this object
     * 
     * @param data - the data payload
     * @param data_length - length of actual used data in bytes - can't be more than maximum allowed
     * by the matching type in EventDataType
     * @param data_type the event type
     * @param callback - user supplied callback to be called when event fired by event manager
     * @param description  as std::string
     */
    SelfEvent(
        EventManager &event_manager,
        EventDataType &data,
        unsigned long data_length,
        EventType data_type,        
        SelfEventCallback callback,
        const std::string& description);

    /**
     * @brief Private ctor - Construct a new Self Event object - 
     * only EventManager can create this object
     * This is an overloading ctor, same field like the above one except the last one which allows 
     * sending an event description as C-style string (or not send a description at all).
     */
    SelfEvent(
        EventManager &event_manager,
        EventDataType &data,
        unsigned long data_length,
        EventType data_type,        
        SelfEventCallback callback, 
        const char* description="");    

    // Do not create empty events
    SelfEvent() = delete;    
    SelfEvent(const SelfEvent&) = delete;
    SelfEvent & operator = (const SelfEvent&) = delete;
    SelfEvent(SelfEvent&&) = delete;
    SelfEvent& operator = (SelfEvent&&) = delete; 
};


//TODO - implement a periodic event later as needed (using event loop timer source support)
//class SelfEventPeriodic : public SelfEvent
//{

//}

//TODO - implement a delayed event later as needed (using event loop timer source support)
//class SelfEventDelayed : public SelfEvent
//{

//}

} // namespace mbl {

#endif // _SelfEvent_h_
