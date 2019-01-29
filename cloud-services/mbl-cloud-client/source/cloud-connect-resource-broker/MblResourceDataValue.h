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

#ifndef MblResourceDataValue_h_
#define MblResourceDataValue_h_

#include <stdint.h>

#include "MblCloudConnectTypes.h"

namespace mbl {

class MblResourceDataValue {
public:

    MblResourceDataValue() = default ;

/**
 * @brief Construct a new Mbl Resource Data Value object and 
 * stores provided string.
 * @param str data that should be stored.
 */
    MblResourceDataValue(const std::string &str)
    : data_type_ (MblCloudConnectResourceDataType::STRING),
      string_data_value_(str) 
    {
    }

/**
 * @brief Construct a new Mbl Resource Data Value object and 
 * stores provided integer.
 * @param integer data that should be stored.
 */
    MblResourceDataValue(int64_t integer)
    : data_type_ (MblCloudConnectResourceDataType::INTEGER),
      integer_data_value_(integer)
    {
    }

    /**
     * @brief Stores provided string. 
     * 
     * @param str data that should be stored.
     */
    void set_value(const std::string &str) 
    {
        assert(data_type_ == MblCloudConnectResourceDataType::INVALID);
        data_type_ = MblCloudConnectResourceDataType::STRING;
        string_data_value_ = str;        
    }

/**
 * @brief Stores provided integer.
 * @param integer data that should be stored.
 */
    void set_value(int64_t integer)
    {
        assert(data_type_ == MblCloudConnectResourceDataType::INVALID);
        data_type_ = MblCloudConnectResourceDataType::INTEGER;
        integer_data_value_ = integer;        
    }

/**
 * @brief Gets the value of stored string.  
 * @return std::string returned value.
 */
    std::string get_value_string() 
    {
        assert(data_type_ == MblCloudConnectResourceDataType::STRING);
        return string_data_value_;        
    }

/**
 * @brief Gets the value of stored integer.  
 * @return int64_t returned value.
 */
    int64_t get_value_integer()
    {
        assert(data_type_ == MblCloudConnectResourceDataType::INTEGER);
        return integer_data_value_;        
    }
    
private:
    // for current moment we use simple implementation for integer and string.
    // When we shall have more types, consider using union or byte array for 
    // storing different types. 
    std::string string_data_value_;
    int64_t integer_data_value_;

    // stores type of stored data
    MblCloudConnectResourceDataType data_type_ = MblCloudConnectResourceDataType::INVALID;

    // currently we don't have pointers in class members, so we can allow default ctor's 
    // and assign operators without worry.  
};

}

#endif