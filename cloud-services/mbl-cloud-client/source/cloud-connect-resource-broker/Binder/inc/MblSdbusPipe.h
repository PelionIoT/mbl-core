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


#ifndef MblSdbusPipe_h_
#define MblSdbusPipe_h_

#include <poll.h>


// bi directional unnamed-pipes mailbox
// fdtab[0] is used to send messages to ccrb thread
// fdtab[1] is used to send replies from ccrb thread
typedef struct MblSdbusPipe {

    int             pipefd[2];    // Store read (0) and write (1) file descriptors for the pipe
    struct pollfd   pollfd[2];    // Store polling file descriptors on the pipe
}MblSdbusPipe;   

typedef struct MblSdbusPipeMsg_dummy {
    char    msg[100];
}MblSdbusPipeMsg_dummy;

// Keep this enumerator
typedef enum MblSdbusPipeMsgType {
    PIPE_MSG_TYPE_DUMMY = 0,      // TODO: delete this one later

    PIPE_MSG_TYPE_LAST      // This must be the last one!
}MblSdbusPipeMsgType;

typedef struct MblSdbusPipeMsg {
    MblSdbusPipeMsgType         msg_type;
    union 
    {
        MblSdbusPipeMsg_dummy   dummy_msg;
    }data;    
}MblSdbusPipeMsg;


int MblSdbusPipe_create(MblSdbusPipe *pipe_object);
int MblSdbusPipe_destroy(MblSdbusPipe *pipe_object);
int MblSdbusPipe_send_message(MblSdbusPipe *pipe_object , MblSdbusPipeMsg *msg) ;
int MblSdbusPipe_message_receive(MblSdbusPipe *pipe_object , MblSdbusPipeMsg **msg);


#endif // MblSdbusPipe_h_