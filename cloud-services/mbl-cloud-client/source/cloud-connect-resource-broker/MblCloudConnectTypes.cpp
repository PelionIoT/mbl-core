/*
 * Copyright (c) 2016-2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: ...
 */

#include <cassert>
#include "MblCloudConnectTypes.h"

namespace mbl {

ResourceData::ResourceData(
    const std::string &path, 
    const ResourceDataType type)
: path_(path),
  data_type_ (type) 
{
    // leave value not initialized
}

ResourceData::ResourceData(
    const std::string &path, 
    const std::string &initial_value)
: path_(path),
  string_value_(initial_value),
  data_type_ (ResourceDataType::STRING) 
{ }

ResourceData::ResourceData(const std::string &path, int64_t initial_value)
: path_(path),
  integer_value_(initial_value),
  data_type_ (ResourceDataType::INTEGER)
{ }

const std::string& ResourceData::get_path() const
{
    return path_;        
}

ResourceDataType ResourceData::get_data_type() const
{
    return data_type_;
}

void ResourceData::set_value(const std::string &value) 
{
    assert(data_type_ == ResourceDataType::STRING);
    string_value_ = value;        
}

void ResourceData::set_value(int64_t value)
{
    assert(data_type_ == ResourceDataType::INTEGER);
    integer_value_ = value;        
}

const std::string& ResourceData::get_value_string() const
{
    assert(data_type_ == ResourceDataType::STRING);
    return string_value_;        
}

int64_t ResourceData::get_value_integer() const
{
    assert(data_type_ == ResourceDataType::INTEGER);
    return integer_value_;        
}

}