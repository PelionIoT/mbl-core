/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CloudConnectTypes_h_
#define CloudConnectTypes_h_

#include "MblError.h"
#include "CloudConnectExternalTypes.h"

#include <inttypes.h>
#include <vector>
#include <string>

#define RETURN_STRINGIFIED_VALUE(ENUM) case ENUM: return #ENUM

// Simple stringify
#define stringify(s) #s

// If you want to stringify the result of expansion of a macro argument, you have to use two 
// levels of macros. (use xstringify())
#define xstringify(s) stringify(s)

// to mark unused variable for the preprocessor
#define UNUSED(x) (void)(x)

namespace mbl {

/**
 * @brief Class that implements resource data value holder. 
 */
class ResourceData {
public:

/**
 * @brief Construct a new Resource Data object with 
 * uninitialized value. 
 * The value should be set in the future, when it is known. 
 * The value should be set strictly according to the type provided 
 * during construction.    
 * 
 * @param path resource path. Path can't be changed in future. 
 * @param type resource value type
 */
    ResourceData(const std::string &path, const ResourceDataType type);

/**
 * @brief Construct a new Resource Data object and stores provided string. 
 * From the moment this object can store only string.
 *  
 * @param path resource path. Path can't be changed in future. 
 * @param initial_value data that should be stored.
 */
    ResourceData(const std::string &path, const std::string &initial_value);

/**
 * @brief Construct a new Resource Data object and stores provided integer. 
 * From the moment this object can store only integer.
 * 
 * @param path resource path. Path can't be changed in future. 
 * @param initial_value data that should be stored.
 */
    ResourceData(const std::string &path, int64_t initial_value);

/**
 * @brief Gets stored resource path.
 * @return std::string resource path.
 */
    std::string get_path() const;

/**
 * @brief Gets resource data type. 
 * @return ResourceDataType resource value data type 
 */
    ResourceDataType get_data_type() const;

/**
 * @brief Stores provided string data. 
 * This API should be used if object was constructed to store string.
 * @param value data that should be stored. 
 */
    void set_value(const std::string &value); 

/**
 * @brief Stores provided integer data.
 * This API should be used if object was constructed to store integer.
 * @param value data that should be stored.
 */
    void set_value(int64_t value);

/**
 * @brief Gets the value of stored string value.  
 * @return std::string returned value.
 */
    std::string get_value_string() const;

/**
 * @brief Gets the value of stored integer value.  
 * @return int64_t returned value.
 */
    int64_t get_value_integer() const;
    
private:
    const std::string path_;

    // for current moment we use simple implementation for integer and string.
    // When we shall have more types, consider using union or byte array for 
    // storing different types. 
    std::string string_value_;
    int64_t integer_value_ = 0xBADBEEF;

    // stores type of stored data
    ResourceDataType event_type_;

    // currently we don't have pointers in class members, so we can allow default ctor's 
    // and assign operators without worry.
};

struct ResourceSetOperation
{
    /**
     * @brief Construct a new container for set operation. 
     * @param input_data input data for set operation
     */
    ResourceSetOperation(
        const ResourceData &input_data)
    : input_data_(input_data) 
    {}

    const ResourceData input_data_; // set operation input data
    CloudConnectStatus output_status_ = ERR_FAILED; // set operation output status
};

struct ResourceGetOperation
{
    /**
     * @brief Construct a new container for get operation. 
     * @param input_path path of the resource who's value is required.
     * @param input_type type of the resource data.
     */
    ResourceGetOperation(
        const std::string &input_path, 
        const ResourceDataType input_type)
    : inout_data_(input_path, input_type) 
    {}

    ResourceData inout_data_;// get operation input and output data
    CloudConnectStatus output_status_ = ERR_FAILED; // get operation output status
};

/**
 * @brief Returns readable explanation of Cloud Connect Status.
 * 
 * @param CloudConnectStatus input status. 
 * @return const char* stringified readable explanation of the status. 
 */
const char* CloudConnectStatus_to_readable_str(const CloudConnectStatus status);

/**
 * @brief Returns D-Bus format string representation of the Cloud Connect Status.
 * 
 * @param CloudConnectStatus input status. 
 * @return const char* D-Bus format string representation of the provided status.
 */
const char* CloudConnectStatus_error_to_DBus_str(const CloudConnectStatus status);

/**
 * @brief Returns stringified value of Cloud Connect Status.
 * 
 * @param CloudConnectStatus that should be stringified. 
 * @return const char* stringified value of CloudConnectStatus. 
 */
const char* CloudConnectStatus_to_str(const CloudConnectStatus status);

/**
 * @brief Returns stringified value of Resource Data Type.
 * 
 * @param ResourceDataType that should be stringified. 
 * @return const char* stringified value of ResourceDataType. 
 */
const char* ResourceDataType_to_str(const ResourceDataType type);

/**
 * @brief This helper new type class defines an "smart" MblError status - it initializes to 
 * MblError::None to mark success. Afterwards, it can be set only one more time to its final value. 
 * Multiple sets afterwards will not influence the 1st set.
 * 
 */
class OneSetMblError
{
public:
    OneSetMblError();

    /**
     * @brief set the value to a new value if it has never been set yet
     * 
     * @param new_val 
     */
    void set(const MblError new_val);

    /**
     * @brief return the current status
     * 
     * @return MblError 
     */
    MblError get();
    
    /**
     * @brief stringify status by calling to MblError_to_str
     * 
     * @return const char* 
     */
    const char* get_status_str();
private:
    MblError err_;
    bool one_time_set_flag_;
};

} //namespace mbl


#endif // CloudConnectTypes_h_
