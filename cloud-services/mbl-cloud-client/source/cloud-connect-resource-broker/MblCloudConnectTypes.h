
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
#include  "MblResourceDataValue.h"

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