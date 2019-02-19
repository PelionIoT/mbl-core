/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _EventManager_h_
#define _EventManager_h_

#include "SelfEvent.h"

#include <systemd/sd-event.h>

#include <inttypes.h>

namespace mbl {

//TODO - extend this class to support delayed events and delayed periodic events
//TODO - consider adding memory pool to avoid dynamic construction of events
/**
 * @brief The events manager is a utility class which allows the upper layer to easily send itself
 * events. It generates event id for debugging and to allow caller to cancel events.
 * events are kept in an std::map events_ as unique_ptr. It is not thread safe and external threads 
 * should not use it at this stage.
 */
class EventManager
{
public:
    EventManager();
    ~EventManager();

    /**
     * @brief initialize the event manager - get reference to event loop
     * 
     * @return MblError - Error::None for success running the loop, otherwise the failure reason
     */
    MblError init();

    /**
     * @brief initialize the event manager - release reference on event loop
     * 
     * @return MblError - Error::None for success running the loop, otherwise the failure reason
     */
    MblError deinit();

    /**
     * @brief 
     * Must be called by CCRB thread only. Send 'defered event' to the event loop using 
     * sd_event_add_defer().
     * For more details: https://www.freedesktop.org/software/systemd/man/sd_event_add_defer.html#
     * @param data - the data to be sent, must be formatted according to the data_type
     * @param data_length - length of data used, must be less than the maximum size allowed
     * @param data_type - the type of data to be sent.
     * @param callback - callback to be called when event is fired
     * @param out_event_id  - generated event id (for debug or to cancel the event)
     * @param description - optional - description for the event cause, can leave empty string
     * @return MblError - return MblError - Error::None for success, therwise the failure reason
     */
    MblError send_event_immediate(
        SelfEvent::EventDataType data,
        unsigned long data_length,
        SelfEvent::EventType data_type,        
        SelfEventCallback callback,
        uint64_t &out_event_id,
        const std::string& description);

    /**
     * @brief - Overloading function - exactly the same as above, but received a description as a 
     * C-style string (can be also completely ignored)
     * @param description - optional - description for the event cause, can leave empty string
     */
    MblError send_event_immediate(
        SelfEvent::EventDataType data,
        unsigned long data_length,
        SelfEvent::EventType data_type,        
        SelfEventCallback callback,
        uint64_t &out_event_id,
        const char *description="");
        
private:
    /**
     * @brief static handler function - called for trigerred (fired) events, it is of type 
     * sd_event_handler_t. the *impl function is the actual implementation inside the object
     * 
     * @param s - the event source which fired
     * @param userdata - userdata
     * @return int - 0 on success, On failure, return a negative Linux errno-style error code.
     */
    static int self_event_handler(sd_event_source *s, void *userdata);
    
    /**
     * @brief called by self_event_handler() on Event manager - this is the actual implementation
     * Handles a fired self event.
     *     
     * @param s - the event source which fired
     * @param ev - the arriving event
     * @return int - 0 for success, else a Linux C style error code.
     */
    int self_event_handler_impl(sd_event_source *s, SelfEvent *ev);
    
    // This map holds all events sent, it holds a pair of (event id, unique_ptr to event)
    // TODO - make sure it is cleaned when manager is destroyed
    std::map< uint64_t , std::unique_ptr<SelfEvent> >  events_;

    // event id to be assigned to the next event created
    uint64_t next_event_id_;

    // a handle to the default event loop of CCRB thread
    sd_event* event_loop_handle_;
};

} //  namespace mbl {

#endif //_EventManager_h_
