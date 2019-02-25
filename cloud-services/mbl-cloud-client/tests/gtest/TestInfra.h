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

#define ARRAY_SIZE(x) ((sizeof (x)) / (sizeof *(x)))

enum TestResult {
    TEST_SUCCESS,
    TEST_FAILED_EXPECTED_RESULT_MISMATCH = -1,
    TEST_FAILED_SD_BUS_SYSTEM_CALL_FAILED = -2,
    TEST_FAILED_ADAPTER_METHOD_FAILED = -3,
    
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
    if(0 == test_result)
    {
        test_result = result;
    }

    return test_result;
}

#endif //#ifndef _TestInfraCommon_h_
