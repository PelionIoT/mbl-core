/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cassert>

#include "CloudConnectTypes.h"

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
