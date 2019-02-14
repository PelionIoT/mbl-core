/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CloudConnectExternalTypes_h_
#define CloudConnectExternalTypes_h_

/**
 * @brief Status of Cloud Connect operations.
 */
enum CloudConnectStatus {
    SUCCESS = 0x0000, 
    FAILED  = 0x0001,

};
 
typedef enum CloudConnectStatus CloudConnectStatus;


/**
 * @brief Cloud Connect resource data type.
 * Currently supported LwM2M resource data types are STRING and INTEGER. 
 */
enum ResourceDataType {
    STRING    = 1,  // supported as array of chars
    INTEGER   = 2,  // supported as UINT64 
    FLOAT     = 3,  // currently not supported
    BOOLEAN   = 4,  // currently not supported
    OPAQUE    = 5,  // currently not supported
    TIME      = 6,  // currently not supported
    OBJLINK   = 7,  // currently not supported
};
    
typedef enum ResourceDataType ResourceDataType;

#endif // CloudConnectExternalTypes_h_
