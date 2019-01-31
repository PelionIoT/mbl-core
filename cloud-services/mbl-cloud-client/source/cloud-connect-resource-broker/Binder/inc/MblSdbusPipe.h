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
#include <stdint.h>

// bi directional unnamed-pipes mailbox
// fdtab[0] is used to send messages to ccrb thread
// fdtab[1] is used to send replies from ccrb thread
typedef struct MblSdbusPipe {

    int             pipefd[2];    // Store read (0) and write (1) file descriptors for the pipe
    struct pollfd   pollfd[2];    // Store polling file descriptors on the pipe
}MblSdbusPipe;   

// TODO : fix all name of this module to DBusAdapterMailBox
int MblSdbusPipe_create(MblSdbusPipe *pipe_object);
int MblSdbusPipe_destroy(MblSdbusPipe *pipe_object);
int MblSdbusPipe_data_send(MblSdbusPipe *pipe_object , uint8_t *data, uint32_t data_size);
int MblSdbusPipe_data_receive(MblSdbusPipe *pipe_object , uint8_t **data);


#endif // MblSdbusPipe_h_