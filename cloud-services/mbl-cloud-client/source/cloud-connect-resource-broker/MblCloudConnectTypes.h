
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


#ifndef MblCloudConnectTypes_h_
#define MblCloudConnectTypes_h_

#include  <stdint.h>
#include  <vector>

#include  "MblError.h"

namespace mbl {

/**
 * @brief Mbl Cloud Connect resource data type.
 * Currently supported LwM2M resource data types. 
 */
    enum class MblCloudConnectResourceDataType {
        INVALID   = 0x0,
        STRING    = 0x1,
        INTEGER   = 0x2,
        FLOAT     = 0x3,
        BOOLEAN   = 0x4,
        OPAQUE    = 0x5,
        TIME      = 0x6,
        OBJLINK   = 0x7,
    };
    
/**
 * @brief Class that implements resource data value holder. 
 */
class MblResourceDataValue {
public:

    MblResourceDataValue();

/**
 * @brief Construct a new Mbl Resource Data Value object and 
 * stores provided string.
 * @param str data that should be stored.
 */
    MblResourceDataValue(const std::string &str);

/**
 * @brief Construct a new Mbl Resource Data Value object and 
 * stores provided integer.
 * @param integer data that should be stored.
 */
    MblResourceDataValue(int64_t integer);

/**
 * @brief Stores provided string. 
 * 
 * @param str data that should be stored.
 */
    void set_value(const std::string &str); 

/**
 * @brief Stores provided integer.
 * @param integer data that should be stored.
 */
    void set_value(int64_t integer);

/**
 * @brief Gets the value of stored string.  
 * @return std::string returned value.
 */
    std::string get_value_string();

/**
 * @brief Gets the value of stored integer.  
 * @return int64_t returned value.
 */
    int64_t get_value_integer();
    
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


/**
 * @brief [resource_path, resource_typed_data_value] tuple.
 */
    struct MblCloudConnect_ResourcePath_Value
    {
        std::string path;
        MblResourceDataValue typed_data_value;
    };

/**
 * @brief [resource_path, resource_data_type] tuple.
 */
    struct MblCloudConnect_ResourcePath_Type
    {
        std::string path;
        MblCloudConnectResourceDataType data_type;
    };

/**
 * @brief [resource_path, resource_typed_data_value, operation_status] tuple.
 */
    struct MblCloudConnect_ResourcePath_Value_Status
    {
        std::string path;
        MblResourceDataValue typed_data_value;
        MblError operation_status;
    };

/**
 * @brief [resource_path, operation_status] tuple.
 */
    struct MblCloudConnect_ResourcePath_Status
    {
        std::string path;
        MblError operation_status;
    };

} //namespace mbl


#endif // MblCloudConnectTypes_h_