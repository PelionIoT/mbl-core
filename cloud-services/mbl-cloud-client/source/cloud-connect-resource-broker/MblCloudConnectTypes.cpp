/*
 * Copyright (c) 2019 ARM Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
: path_(path)
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