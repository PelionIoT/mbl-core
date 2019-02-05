/*
* Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include "mbed-trace/mbed_trace.h"
#include <gtest/gtest.h>
#include "MblCloudClient.h"

#include MBED_CLOUD_CLIENT_USER_CONFIG_FILE

M2MObjectList m2m_object_list;

M2MBase::Mode mode;
M2MBase::Operation operation;
M2MResourceBase::ResourceType type;

#define TRACE_GROUP "mbl"

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv); 
     
    tr_debug("test 123 %d ", (int)mode);
    
    return RUN_ALL_TESTS();
}

