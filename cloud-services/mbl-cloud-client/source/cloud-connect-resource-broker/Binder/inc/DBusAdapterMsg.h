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

#ifndef _DBusAdapterMsg_h_
#define _DBusAdapterMsg_h_

namespace mbl{

#define DBUS_MAX_MSG_RAW_PAYLOAD_SIZE        100

/* In Linux versions before 2.6.11, the capacity of a pipe was the same as the system page size 
(e.g., 4096 bytes on i386). Since Linux 2.6.11, the pipe capacity is 65536 bytes.
Simplify things here : allow maximum size of 4096 for now */
//#define DBUS_MAX_MSG_SIZE   65536
//#define DBUS_MAX_MSG_PAYLOAD_SIZE \
    //(DBUS_MAX_MSG_SIZE - \
    //(sizeof(struct DBusAdapterMsg) - sizeof(union DBusAdapterMsgPayload)))


//TODO :  add 2 static compile time casts to check that 
//DBUS_MAX_MSG_RAW_PAYLOAD_SIZE<DBUS_MAX_MSG_SIZE and that max size of DBusAdapterMsg
// is less than  MAX_MSG_SIZE
typedef struct DBusAdapterMsg_raw_ 
{
    char    bytes[DBUS_MAX_MSG_RAW_PAYLOAD_SIZE];
}DBusAdapterMsg_raw;

// Keep this enumerator
typedef enum DBusAdapterMsgType_
{
    DBUS_ADAPTER_MSG_RAW = 1,      // TODO: delete this one later ?
    DBUS_ADAPTER_MSG_EXIT = 2,     // exit

    DBUS_ADAPTER_MSG_LAST = 0x7FFFFFFF
}DBusAdapterMsgType;

typedef union DBusAdapterMsgPayload_
{
    DBusAdapterMsg_raw  raw;
}DBusAdapterMsgPayload; 


struct DBusAdapterMsg
{   
    uint64_t get_sequence_num() { return sequence_num; } // TODO : create file?
    
    uint32_t                payload_len;                 // size in bytes of payload field
    DBusAdapterMsgType      type;  

    DBusAdapterMsgPayload   payload;

    private:
    friend class DBusAdapterMailbox;
    uint64_t     sequence_num;
};

}//namespace mbl
#endif // _DBusAdapterMsg_h_