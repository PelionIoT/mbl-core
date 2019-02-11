/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TestInfraCommon_h_
#define _TestInfraCommon_h_


#define GTEST_LOG_START_TEST \
    tr_debug("Starting %s : %s", \
    __PRETTY_FUNCTION__, \
    ::testing::UnitTest::GetInstance()->current_test_info()->name())

// In some tests, for simplicity, threads are not synchronized
// In the real code they are -> using the event loop or other means. That's why the actual mailbox code
// does not implement retries on timeout polling failures.
// Since they are synchronized, read should always succeed.
// That's why I wait up to 100 ms
#define TI_DBUS_MAILBOX_MAX_WAIT_TIME_MS 100

#endif //#ifndef _TestInfraCommon_h_