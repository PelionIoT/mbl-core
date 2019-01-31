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


#ifndef CloudConnectTypes_h_
#define CloudConnectTypes_h_

extern "C" {

/**
 * @brief Status of Cloud Connect operations.
 */
enum CloudConnectStatus {
    SUCCESS = 0x0000, 
    FAILED  = 0x0001,
    
    // allign enumerators to be 32 bits 
    MAX_STATUS = 0xFFFFFFFF
};
 
typedef enum CloudConnectStatus CloudConnectStatus;

}

#endif // CloudConnectTypes_h_