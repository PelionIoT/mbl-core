/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cassert>

#include "CloudConnectTypes.h"
#include "mbed-trace/mbed_trace.h"

#define TRACE_GROUP "ccrb"

namespace mbl {

ResourceData::ResourceData(
    const std::string &path, 
    const ResourceDataType type)
: path_(path),
  data_type_(type) 
{
    tr_debug("%s", __PRETTY_FUNCTION__);    
    // leave value not initialized
}

ResourceData::ResourceData(
    const std::string &path, 
    const std::string &initial_value)
: path_(path),
  string_value_(initial_value),
  data_type_(ResourceDataType::STRING) 
{
    tr_debug("%s", __PRETTY_FUNCTION__);    
}

ResourceData::ResourceData(const std::string &path, int64_t initial_value)
: path_(path),
  integer_value_(initial_value),
  data_type_(ResourceDataType::INTEGER)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

std::string ResourceData::get_path() const
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    return path_;        
}

ResourceDataType ResourceData::get_data_type() const
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    return data_type_;
}

void ResourceData::set_value(const std::string &value) 
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(data_type_ == ResourceDataType::STRING);
    string_value_ = value;        
}

void ResourceData::set_value(int64_t value)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(data_type_ == ResourceDataType::INTEGER);
    integer_value_ = value;        
}

std::string ResourceData::get_value_string() const
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(data_type_ == ResourceDataType::STRING);
    return string_value_;        
}

int64_t ResourceData::get_value_integer() const
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(data_type_ == ResourceDataType::INTEGER);
    return integer_value_;        
}

}

const char* CloudConnectStatus_to_readable_string(const CloudConnectStatus status)
{
    switch (status)
    {
        case SUCCESS: 
            return "SUCCESS";

        case FAILED: 
            return "FAILED";

        default:
            return "Unknown Cloud Connect Status";
    }
}

const char* CloudConnectStatus_stringify(const CloudConnectStatus status)
{
    switch (status)
    {
        RETURN_STRINGIFIED_VALUE(SUCCESS); 
        RETURN_STRINGIFIED_VALUE(FAILED); 

        default:
            return "Unknown Cloud Connect Status";
    }
}

const char* ResourceDataType_stringify(const ResourceDataType type)
{
    switch (type)
    {
        RETURN_STRINGIFIED_VALUE(STRING); 
        RETURN_STRINGIFIED_VALUE(INTEGER); 
        RETURN_STRINGIFIED_VALUE(FLOAT); 
        RETURN_STRINGIFIED_VALUE(BOOLEAN); 
        RETURN_STRINGIFIED_VALUE(OPAQUE); 
        RETURN_STRINGIFIED_VALUE(TIME); 
        RETURN_STRINGIFIED_VALUE(OBJLINK); 

        default:
            return "Unknown Resource Data Type";
    }
}
