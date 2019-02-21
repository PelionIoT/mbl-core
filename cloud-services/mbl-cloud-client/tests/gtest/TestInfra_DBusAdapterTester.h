/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TestInfra_DBusAdapterTester_h_
#define _TestInfra_DBusAdapterTester_h_

#include <systemd/sd-event.h>

#include "MblError.h"
#include "DBusAdapter.h"

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
    TestInfra_DBusAdapterTester(mbl::DBusAdapter &adapter) : 
      adapter_(adapter) 
      {};

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
    mbl::MblError event_loop_run(mbl::MblError &stop_status, mbl::MblError expected_stop_status);

    /**
     * @brief Get the event loop handle object
     * 
     * @return sd_event* 
     */
    sd_event *get_event_loop_handle();    
    
    /**
     * @brief calls sd_event_add_defer, pass userdata and a handler
     * use this call only if calling thread is the one to initialize the adapter!
     * @param handler - a handler to be called by the event loop
     * @param userdata - a data to be passed when handler is called
     * @return int - see sd_event_add_defer()
     */
    int send_event_defer(sd_event_handler_t handler, void *userdata);

    /**
     * @brief Invokes EventManager::send_event_immediate
     */
    mbl::MblError send_event_immediate(
        mbl::SelfEvent::EventData data,
        unsigned long data_length,
        mbl::SelfEvent::EventDataType data_type,        
        mbl::SelfEventCallback callback,
        uint64_t &out_event_id,
        const char* description="");

private:
    mbl::DBusAdapter &adapter_;
};

#endif //#ifndef _TestInfra_DBusAdapterTester_h_
