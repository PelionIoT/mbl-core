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

MblResourceDataValue::MblResourceDataValue(const std::string &str)
: string_data_value_(str),
  data_type_ (MblResourceDataType::STRING) 
{
}

MblResourceDataValue::MblResourceDataValue(int64_t integer)
: integer_data_value_(integer),
  data_type_ (MblResourceDataType::INTEGER)
{
}

MblResourceDataType get_resource_data_value_type();
{
    return data_type_;
}

void MblResourceDataValue::set_value(const std::string &str) 
{
    assert(data_type_ == MblResourceDataType::INVALID);
    data_type_ = MblResourceDataType::STRING;
    string_data_value_ = str;        
}

void MblResourceDataValue::set_value(int64_t integer)
{
    assert(data_type_ == MblResourceDataType::INVALID);
    data_type_ = MblResourceDataType::INTEGER;
    integer_data_value_ = integer;        
}

std::string MblResourceDataValue::get_value_string() 
{
    assert(data_type_ == MblResourceDataType::STRING);
    return string_data_value_;        
}

int64_t MblResourceDataValue::get_value_integer()
{
    assert(data_type_ == MblResourceDataType::INTEGER);
    return integer_data_value_;        
}

}