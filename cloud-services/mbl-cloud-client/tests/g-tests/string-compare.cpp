/*
* Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include <gtest/gtest.h> // googletest header file
#include <string>
//#include "cloud-connect-resource-broker/MblCloudConnectResourceBroker.h"

using std::string;

const char *actualValTrue  = "hello gtest";
const char *actualValFalse = "hello world";
const char *expectVal      = "hello gtest";

//MblCloudConnectResourceBroker cloud_connect_resource_broker;

TEST(StrCompare, CStrEqual) {
    EXPECT_STREQ(expectVal, actualValTrue);
}

TEST(StrCompare, CStrNotEqual) {
    EXPECT_STREQ(expectVal, actualValFalse);
}

