/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TestInfraCommon_h_
#define _TestInfraCommon_h_

#include "systemd/sd-bus.h"
#include "systemd/sd-event.h"

#include "MblError.h"
#include "CloudConnectTrace.h"

#define GTEST_LOG_START_TEST \
    TR_DEBUG("Starting Test : %s", \
    ::testing::UnitTest::GetInstance()->current_test_info()->name())

#define TESTER_VALIDATE_EQ(a, b) \
    if (a != b)                  \
    return MblError::DBA_InvalidValue

#endif //#ifndef _TestInfraCommon_h_