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
    ERR_FIRST = 0x1000,

    // TODO: required to pass over all ERR_INTERNAL_ERROR codes and check if any of them are 
    // recoverable. For the recoverable ones, we might need other return code. 
    // We suspect that most of them are not recoverable(fatal). In that case, 
    // few things are important when having a fatal error :
    // 1) We must cause the client thread to get out from blocking state (if it called blocking)
    // to allow it response faster after we restart.
    // 2) We must send self-exit request to deallocate all resources.
    // 3) We must exist in the coordination with the systemd watchdog daemon (whenever we will
    // have such one). There is an API to the systemd watchdog, need to research.
    ERR_INTERNAL_ERROR                              = ERR_FIRST,
    ERR_INVALID_APPLICATION_RESOURCES_DEFINITION    = 0x1001,
    ERR_REGISTRATION_ALREADY_IN_PROGRESS            = 0x1002,
    ERR_ALREADY_REGISTERED                          = 0x1003,
    ERR_INVALID_ACCESS_TOKEN                        = 0x1004,
    ERR_INVALID_RESOURCE_PATH                       = 0x1005,
    ERR_RESOURCE_NOT_FOUND                          = 0x1006,
    ERR_INVALID_RESOURCE_TYPE                       = 0x1007,
    ERR_NOT_SUPPORTED                               = 0x1008,
	ERR_NUM_ALLOWED_CONNECTIONS_EXCEEDED            = 0x1009
};
 
typedef enum CloudConnectStatus CloudConnectStatus;

static inline bool is_CloudConnectStatus_not_error(const CloudConnectStatus val){
    return val >= STATUS_SUCCESS && val < ERR_FIRST;
}

static inline bool is_CloudConnectStatus_error(const CloudConnectStatus val){
    return val >= ERR_FIRST;
}

// com.mbed.Pelion1.Connect.Error definitions
#define CLOUD_CONNECT_ERR_INTERNAL_ERROR \
    "com.mbed.Pelion1.Connect.Error.InternalError"
#define CLOUD_CONNECT_ERR_INVALID_APPLICATION_RESOURCES_DEFINITION \
    "com.mbed.Pelion1.Connect.Error.InvalidApplicationResourceDefinition"
#define CLOUD_CONNECT_ERR_REGISTRATION_ALREADY_IN_PROGRESS \
    "com.mbed.Pelion1.Connect.Error.RegistrationAlreadyInProgress"
#define CLOUD_CONNECT_ERR_ALREADY_REGISTERED \
    "com.mbed.Pelion1.Connect.Error.AlreadyRegistered"
#define CLOUD_CONNECT_ERR_INVALID_RESOURCE_PATH \
	"com.mbed.Pelion1.Connect.Error.InvalidResourcePath"
#define CLOUD_CONNECT_ERR_RESOURCE_NOT_FOUND \
	"com.mbed.Pelion1.Connect.Error.ResourceNotFound"
#define CLOUD_CONNECT_ERR_INVALID_RESOURCE_TYPE \
	"com.mbed.Pelion1.Connect.Error.InvalidResourceType"
#define CLOUD_CONNECT_ERR_INVALID_ACCESS_TOKEN \
	"com.mbed.Pelion1.Connect.Error.InvalidAccessToken"
#define CLOUD_CONNECT_ERR_NOT_SUPPORTED \
	"mbed.Cloud.Connect.Error.NotSupported"
#define CLOUD_CONNECT_ERR_NUM_ALLOWED_CONNECTIONS_EXCEEDED \
    "com.mbed.Pelion1.Connect.Error.NumAllowedConnectionsExceeded"
    
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
