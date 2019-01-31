/*
 * Copyright (c) 2016-2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: ...
 */

#ifndef CloudConnectTypes_h_
#define CloudConnectTypes_h_

extern "C" {

/**
 * @brief Status of Cloud Connect operations.
 */
enum CloudConnectStatus {
    SUCCESS = 0x0000, 
    FAILED  = 0x0001,
    
    // allign enumerators to be 32 bits 
    MAX_STATUS = 0xFFFFFFFF
};
 
typedef enum CloudConnectStatus CloudConnectStatus;

}

#endif // CloudConnectTypes_h_