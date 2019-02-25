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
    // Status range (non error statuses)
    // Start all enums in status range with "STATUS_" prefix
    STATUS_SUCCESS = 0x0000, 

    // Error range
    // Start all enums in this range with "ERR_" prefix
    FIRST_ERROR = 0x1000,
    ERR_FAILED                                      = FIRST_ERROR,
    ERR_INTERNAL_ERROR                              = 0x1001,	
    ERR_INVALID_APPLICATION_RESOURCES_DEFINITION    = 0x1001,
    ERR_REGISTRATION_ALREADY_IN_PROGRESS            = 0x1002,
    ERR_ALREADY_REGISTERED                          = 0x1003,
};
 
typedef enum CloudConnectStatus CloudConnectStatus;

static inline bool is_CloudConnectStatus_not_error(const CloudConnectStatus val){
    return val >= STATUS_SUCCESS && val < FIRST_ERROR;
}

static inline bool is_CloudConnectStatus_error(const CloudConnectStatus val){
    return val >= FIRST_ERROR;
}

// mbed.Cloud.Connect DBus errors definitions
#define CLOUD_CONNECT_ERR_FAILED "mbed.Cloud.Connect.Failed"
#define CLOUD_CONNECT_ERR_INTERNAL_ERROR "mbed.Cloud.Connect.InternalError"
#define CLOUD_CONNECT_ERR_INVALID_APPLICATION_RESOURCES_DEFINITION "mbed.Cloud.Connect.InvalidApplicationresourceDefinition"
#define CLOUD_CONNECT_ERR_REGISTRATION_ALREADY_IN_PROGRESS "mbed.Cloud.Connect.RegistrationAlreadyInProgress"
#define CLOUD_CONNECT_ERR_ALREADY_REGISTERED "mbed.Cloud.Connect.AlreadyRegistered"

/** 
 * @brief Returns corresponding D-Bus format error. 
 * The error returned is a string in a D-Bus format that corresponds to provided enum. 
 */
#define RETURN_DBUS_FORMAT_ERROR(ENUM) case ENUM: return CLOUD_CONNECT_ ## ENUM

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
