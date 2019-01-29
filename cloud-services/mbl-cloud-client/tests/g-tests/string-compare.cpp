/*
* Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*/

//#include "MblCloudClient.h"
#include "cloud-connect-resource-broker/MblCloudConnectResourceBroker.h"
#include <gtest/gtest.h> // googletest header file
#include <string>

using std::string;

const char *actualValTrue  = "hello gtest";
const char *actualValFalse = "hello world";
const char *expectVal      = "hello gtest";

//MblCloudClient::run()

TEST(StrCompare, CStrEqual) {
    EXPECT_STREQ(expectVal, actualValTrue);
}

TEST(StrCompare, CStrNotEqual) {
    EXPECT_STREQ(expectVal, actualValFalse);
}

