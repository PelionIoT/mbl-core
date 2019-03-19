/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TestInfra_DBusAdapterTester_h_
#define _TestInfra_DBusAdapterTester_h_

#include "DBusAdapter.h"
#include "DBusAdapter_Impl.h"
#include "MblError.h"
#include "Event.h"
#include "EventImmediate.h"
#include "EventPeriodic.h"

typedef struct sd_event sd_event;
typedef int (*sd_event_handler_t)(sd_event_source* s, void* userdata);


struct EventData_Raw
{
    char bytes[100];
};

/**
 * @brief This calls allow test body to access private members of DBusAdapter and DBusAdapterImpl
 * Both classes declare this class as friend to allow private members access.
 * The class not only acts as a pipe, it also validates some of the test logic , using macro
 * TESTER_VALIDATE_EQ
 */
class TestInfra_DBusAdapterTester
{
public:
    /**
     * @brief Construct a new TestInfra_DBusAdapterTester object
     *
     * @param adapter -an adapter object to access it's private members during test
     */
    TestInfra_DBusAdapterTester(mbl::DBusAdapter& adapter) : adapter_(adapter){};

    /**
     * @brief Validate adapater is deinitialized
     *
     * @return MblError
     */
    mbl::MblError validate_deinitialized_adapter();

    /**
     * @brief calls DBusAdapterImpl::event_loop_request_stop
     *
     * @param stop_status - the stop status code - reason for stop
     * @return MblError - pipe out the returned value of DBusAdapterImpl::event_loop_request_stop
     */
    mbl::MblError event_loop_request_stop(mbl::MblError stop_status);

    /**
     * @brief calls DBusAdapterImpl::event_loop_run and validates returned value according to given
     * expected_stop_status
     *
     * @param stop_status - the reason for the run to stop, can be MblError:None if normal exit
     * @param expected_stop_status - validate the stop_status to this status
     * @return MblError - if all validations succeed - return MblError:None
     */
    mbl::MblError event_loop_run(mbl::MblError& stop_status, mbl::MblError expected_stop_status);

    /**
     * @brief Get the event loop handle object
     *
     * @return sd_event*
     */
    sd_event* get_event_loop_handle();

    /**
     * @brief calls sd_event_add_defer, pass userdata and a handler
     * use this call only if calling thread is the one to initialize the adapter!
     * @param handler - a handler to be called by the event loop
     * @param userdata - a data to be passed when handler is called
     * @return int - see sd_event_add_defer()
     */
    int send_event_defer(sd_event_handler_t handler, void* userdata);

    /**
     * @brief Invokes EventManager::send_event_immediate
     */
    template <typename T>
    std::pair<mbl::MblError, uint64_t> send_event_immediate(T & data,
                                                            unsigned long data_length,
                                                            mbl::Event::UserCallback callback,
                                                            const std::string& description="")
    {
        return adapter_.impl_->event_manager_.send_event_immediate(
            data, data_length, callback, std::string(description));
    }

    /**
     * @brief Invokes EventManager::send_event_periodic
     */
    std::pair<mbl::MblError, uint64_t>
    send_event_periodic(EventData_Raw & data,
                        unsigned long data_length,
                        mbl::Event::UserCallback callback,
                        uint64_t period_millisec,
                        const std::string& description="")
    {
        return adapter_.impl_->event_manager_.send_event_periodic(
            data, data_length, callback, period_millisec, std::string(description));
    }


    /**
     * @brief Calls sd_event_source_unref
     * @param ev - an event, which sd_event_source will be unreferenced
     *
     * @return void
     */
    void unref_event_source(mbl::Event* ev);

    /**
     * @brief Returns event manager callback
     * @param ev - an event
     *
     * @return mbl::Event::EventManagerCallback
     */
    const mbl::Event::EventManagerCallback get_event_manager_callback(mbl::Event* ev) const;

    /**
     * @brief calls DBusAdapterImpl::bus_enforce_single_connection          
     */
    inline bool bus_enforce_single_connection(std::string& source){
      return adapter_.impl_->bus_enforce_single_connection(source);    
    }

    /**
     * @brief 
     * 
     */
    inline int bus_track_add_dummy_sender(const char * sender){
      adapter_.impl_->connections_tracker_.insert(make_pair(sender, nullptr));
      return 0;    
    }

private:
    mbl::DBusAdapter& adapter_;
};

#endif //#ifndef _TestInfra_DBusAdapterTester_h_
