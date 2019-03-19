/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _Event_h_
#define _Event_h_

#include "MblError.h"
#include "CloudConnectTrace.h"

#include <systemd/sd-event.h>

#include <chrono>
#include <functional>
#include <sstream>

using namespace std::chrono;

typedef struct sd_event sd_event;
typedef struct sd_event_source sd_event_source;

// to be used by Google Test testing
class TestInfra_DBusAdapterTester;

namespace mbl
{

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
    typedef std::function<MblError(sd_event_source*, Event*)> UserCallback;
    typedef std::function<int(sd_event_source*, Event* ev)> EventManagerCallback;

    virtual ~Event();

    template<typename T>
    void unpack_data(T & data)
    {
        mbed_tracef(TRACE_LEVEL_DEBUG, "ccrb-event", "Enter");

        static_assert(std::is_trivial<T>::value && std::is_standard_layout<T>::value,
            "None POD types are not supported with this function!");

        decltype(sizeof(T)) size = sizeof(T);
        serializer_.read(reinterpret_cast<char*>(&data), size);
    }

    //inline virtual const EventData& get_data() const { return data_; }
    inline virtual uint64_t get_id() const { return id_; }
    inline virtual std::string get_description() const { return description_; }
    inline virtual const std::chrono::milliseconds get_creation_time() const
    {
        return creation_time_;
    }
    inline virtual std::chrono::milliseconds get_send_time() const { return send_time_; }
    inline virtual std::chrono::milliseconds get_fire_time() const { return fire_time_; }
    inline virtual sd_event_source* get_sd_event_source() const { return sd_event_source_; }

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
    // event data is serialized into a standard library string buffer
    std::stringstream serializer_;

    // length in bytes of data_
    const unsigned long data_length_;

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
    template <typename T>
    Event(T& data,
             unsigned long data_length,
             UserCallback user_callback,
             EventManagerCallback event_manager_callback,
             sd_event* event_loop_handle,
             std::string description)
    :     
        data_length_(data_length),
        user_callback_(user_callback),
        event_manager_callback_(std::move(event_manager_callback)),
        description_(std::string(description)),
        creation_time_(duration_cast<milliseconds>(system_clock::now().time_since_epoch())),
        fire_time_(0),
        send_time_(0),
        sd_event_source_(nullptr),
        event_loop_handle_(event_loop_handle)
    {
        mbed_tracef(TRACE_LEVEL_DEBUG, "ccrb-event", "Enter");
        
        assert(user_callback);
        assert(data_length_ <= sizeof(data)); // don't assert by type, just avoid corruption
        assert(event_loop_handle_);

        pack_data<T>(data);

        static uint64_t id = 1;
        id_ = id++;

        // get event_loop_handle - this is done in order to get next experation time of the event loop
        sd_event_ref(event_loop_handle_);
    }

    template<typename T>
    void pack_data(T const& data)
    {
        mbed_tracef(TRACE_LEVEL_DEBUG, "ccrb-event", "Enter");

        // this only works on built in data types (PODs)
        static_assert(std::is_trivial<T>::value && std::is_standard_layout<T>::value,
            "None POD types are not supported with this function!");

        serializer_.write(reinterpret_cast<char const*>(&data), sizeof(T));
    }    
};

} // namespace mbl


#endif // _Event_h_
