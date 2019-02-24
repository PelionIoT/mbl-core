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
    // Status range (non error statuses)
    // Start all enums in status range with "STATUS_" prefix
    STATUS_SUCCESS = 0x0000, 

    // Error range
    // Start all enums in this range with "ERR_" prefix
    ERR_FAILED                                      = 0x1000,
    ERR_INVALID_APPLICATION_RESOURCES_DEFINITION    = 0x1001,
    ERR_REGISTRATION_ALREADY_IN_PROGRESS            = 0x0002,
    ERR_ALREADY_REGISTERED                          = 0x0003,
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

}

#endif // CloudConnectExternalTypes_h_
