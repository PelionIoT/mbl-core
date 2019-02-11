/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CloudConnectExternalTypes_h_
#define CloudConnectExternalTypes_h_

extern "C" {

/**
 * @brief Status of Cloud Connect operations.
 */
enum CloudConnectStatus {
    SUCCESS = 0x0000, 
    FAILED  = 0x0001,

};
 
typedef enum CloudConnectStatus CloudConnectStatus;

/**
 * @brief Returns stringified value of CloudConnectStatus.
 * 
 * @param CloudConnectStatus that should be stringified. 
 * @return const char* stringified value of CloudConnectStatus. 
 */
const char* CloudConnectStatus_to_string(CloudConnectStatus status);

}

#endif // CloudConnectExternalTypes_h_
