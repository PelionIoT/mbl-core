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

#ifndef MblSdbusPipeMsg_h_
#define MblSdbusPipeMsg_h_

#define RAW_MSG_SIZE 100

typedef struct MblSdbusPipeMsg_raw {
    char    bytes[RAW_MSG_SIZE];
}MblSdbusPipeMsg_raw;

// Keep this enumerator
typedef enum MblSdbusPipeMsgType {
    PIPE_MSG_TYPE_RAW = 0,      // TODO: delete this one later ?
    PIPE_MSG_TYPE_EXIT = 1,     // exit

    PIPE_MSG_TYPE_LAST          // This must be the last one!
}MblSdbusPipeMsgType;

typedef struct MblSdbusPipeMsg {
    MblSdbusPipeMsgType     type;
    union 
    {
        MblSdbusPipeMsg_raw  raw;
    }msg;    
}MblSdbusPipeMsg;


#endif // MblSdbusPipeMsg_h_