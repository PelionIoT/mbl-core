/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TestInfraCommon_h_
#define _TestInfraCommon_h_

#include "systemd/sd-bus.h"
#include "systemd/sd-event.h"

#include "CloudConnectTrace.h"
#include "MblError.h"

#define GTEST_LOG_START_TEST                                                                       \
    TR_DEBUG("Starting Test : %s", ::testing::UnitTest::GetInstance()->current_test_info()->name())

#define TESTER_VALIDATE_EQ(a, b)                                                                   \
    if (a != b) return MblError::DBA_InvalidValue

enum TestResult {
    TEST_FAILED,
    TEST_SUCCESS,
    TEST_FAILED_EXPECTED_RESULT_MISMATCH = -1,
    TEST_FAILED_SD_BUS_SYSTEM_CALL_FAILED = -2,
    TEST_FAILED_ADAPTER_METHOD_FAILED = -3,
    TEST_FAILED_INVALID_TEST_PARAMETERS = -4, // invalid test input
};
typedef enum TestResult TestResult;

/**
 * @brief Sets test_result according to the result provided.
 * If test_result was already set to an error, does nothing.
 * 
 * @param test_result current test result.
 * @param result new test result.
 * @return int value of test_result after the set. 
 */
inline static int set_test_result(int & test_result, int result)
{
    if(TEST_SUCCESS == test_result)
    {
        test_result = result;
    }

    return test_result;
}

#endif //#ifndef _TestInfraCommon_h_
