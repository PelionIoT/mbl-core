/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TestsInfra_Tester_h_
#define _TestsInfra_Tester_h_

#include <systemd/sd-event.h>

#include "MblError.h"
#include "DBusAdapter.h"

using namespace mbl;

class DBusAdapterTester
{
  public:
    DBusAdapterTester(DBusAdapter &adapter) : adapter_(adapter) {};

    MblError validate_deinitialized_adapter();
    MblError event_loop_request_stop(DBusAdapterStopStatus stop_status);
    MblError event_loop_run(DBusAdapterStopStatus &stop_status, DBusAdapterStopStatus expected_stop_status);
    sd_event *get_event_loop_handle();

    //use this call only if calling thread is the one to initialize the adapter!
    int add_event_defer(sd_event_handler_t handler, void *userdata);

    DBusAdapter &adapter_;
};

#endif //#ifndef _TestsInfra_Tester_h_