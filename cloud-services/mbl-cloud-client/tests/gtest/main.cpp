/*
* Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include <gtest/gtest.h>
#include "log.h"
#include "mbed-trace/mbed_trace.h"
#include "MblError.h"

using namespace mbl;
#define TRACE_GROUP "ccrb-gtest"

int main(int argc, char **argv) {

    ::testing::InitGoogleTest(&argc, argv);

    // Init Cloud Client log
    MblError retval = log_init();
    if(Error::None != retval){
        printf("Error init Cloud Client log: %s", MblError_to_str(retval));
    }

    tr_debug("%s", __PRETTY_FUNCTION__);

    // Make sure that trace level is debug as we want as much information posibble when running tests
    // in case error occured. This ovvirides mbl-cloud-client default trace level.
    mbed_trace_config_set(TRACE_ACTIVE_LEVEL_DEBUG);

    return RUN_ALL_TESTS();
}

