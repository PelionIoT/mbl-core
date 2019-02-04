/*
* Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include <gtest/gtest.h> // googletest header file
#include <string>
#include "systemd/sd-bus.h"
#include "systemd/sd-event.h"

sd_event * e;
sd_event_new(&e);



using std::string;

const char *actualValTrue  = "hello gtest";
const char *actualValFalse = "hello world";
const char *expectVal      = "hello gtest";


TEST(StrCompare, CStrEqual) {
    EXPECT_STREQ(expectVal, actualValTrue);
}

TEST(StrCompare, CStrNotEqual) {
    EXPECT_STREQ(expectVal, actualValFalse);
}

