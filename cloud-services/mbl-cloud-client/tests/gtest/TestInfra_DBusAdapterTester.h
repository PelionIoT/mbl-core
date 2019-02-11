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

using namespace mbl;

class TestInfra_DBusAdapterTester
{
  public:
    TestInfra_DBusAdapterTester(DBusAdapter &adapter) : adapter_(adapter) {};

    MblError validate_deinitialized_adapter();
    MblError event_loop_request_stop(MblError stop_status);
    MblError event_loop_run(MblError &stop_status, MblError expected_stop_status);
    sd_event *get_event_loop_handle();

    //use this call only if calling thread is the one to initialize the adapter!
    int send_event_defer(sd_event_handler_t handler, void *userdata);

    DBusAdapter &adapter_;
};

#endif //#ifndef _TestInfra_DBusAdapterTester_h_