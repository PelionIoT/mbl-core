/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _Event_h_
#define _Event_h_

#include "MblError.h"

#include <chrono>
#include <functional>
#include <string>

typedef struct sd_event sd_event;
typedef struct sd_event_source sd_event_source;

// to be used by Google Test testing
class TestInfra_DBusAdapterTester;

namespace mbl
{

#define MAX_SIZE_EVENT_DATA_RAW 100

class EventManager;

/**
 * @brief This class is a pure virtual base class for the other event classes which inherit it. It
 * does not support sending events by external threads. Events are sent using EventManager class.
 * Currently, one-time-immidiate and timed out periodic events are supported. Use inheritance to
 * support more types of events.
 */
class Event
{
    // Google test friend class (for tests to be able to reach private members)
    friend class ::TestInfra_DBusAdapterTester;

public:
    typedef std::function<MblError(sd_event_source*, const Event*)> UserCallback;
    typedef std::function<int(sd_event_source*, Event* ev)> EventManagerCallback;

    /**
     * @brief  The event data is a union with structs that define all possible
     * event formats (use only Plain Old data Types).
     * To support variable size - use std containers. if maximal data size is known - developer may
     * use plain old data types.
     *
     */
    typedef union EventData_
    {
        // use this struct when data_type == EventType::RAW
        static const int MAX_BYTES = 100;
        struct EventData_Raw
        {
            char bytes[MAX_BYTES];
        } raw;
    } EventData;

    enum class EventDataType
    {
        RAW = 1,
    };

    virtual ~Event();

    // Getters - inline implemented
    inline virtual const EventData& get_data() const { return data_; }
    inline virtual uint64_t get_id() const { return id_; }
    inline virtual EventDataType get_data_type() const { return data_type_; };
    inline virtual std::string get_description() const { return description_; }
    inline virtual const std::chrono::milliseconds get_creation_time() const
    {
        return creation_time_;
    }
    inline virtual std::chrono::milliseconds get_send_time() const { return send_time_; }
    inline virtual std::chrono::milliseconds get_fire_time() const { return fire_time_; }
    inline virtual sd_event_source* get_sd_event_source() const { return sd_event_source_; }

    // Getters - implemented in cpp
    static const char* EventType_to_str(EventDataType type);
    virtual const char* get_data_type_str();

    /**
     * @brief send event to the sd event loop - pure virtual method
     *
     * @return result - 0 on success, non 0 on error
     */
    virtual int send() = 0;

    /**
     * @brief execute callback and other post send actions
     *
     * @return void
     */
    virtual void on_fire();

protected:
    // event data, may be empty
    const EventData data_;

    // length in bytes of data_
    const unsigned long data_length_;

    // the event type
    const EventDataType data_type_;

    // user callback
    const UserCallback user_callback_;

    // event manager callback - called after event is fired
    const EventManagerCallback event_manager_callback_;

    // user may supply both string type which are kept as std:string
    const std::string description_;

    // event id
    uint64_t id_;

    // creation, send and fire times are recorded
    const std::chrono::milliseconds creation_time_;
    std::chrono::milliseconds fire_time_;
    std::chrono::milliseconds send_time_;

    // sd event source
    sd_event_source* sd_event_source_;

    // pointer to the event loop object
    sd_event* event_loop_handle_;

    /**
     * @brief Private ctor - Construct a new Event object.
     *
     * @param data - the data payload
     * @param data_length - length of actual used data in bytes - can't be more than maximum allowed
     * by the matching type in EventDataType
     * @param data_type the event type
     * @param user_cb - user supplied callback to be called when event fired by event manager
     * @param event_manager_cb - callback to the event manager unmanage_event() method
     * @param description  as std::string
     */
    Event(EventData& data,
          unsigned long data_length,
          EventDataType data_type,
          UserCallback user_callback,
          EventManagerCallback event_manager_callback,
          sd_event* event_loop_handle,
          std::string description);
};

} // namespace mbl

#endif // _Event_h_
