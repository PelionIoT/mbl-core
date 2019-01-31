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

#define _GNU_SOURCE             /* See feature_test_macros(7) */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>              /* Obtain O_* constant definitions */
#include <unistd.h>

#include "MblSdbusPipe.h"

#define READ                        0
#define WRITE                       1
#define MAX_TIME_TO_POLL_MILLISEC   10

/* In Linux versions before 2.6.11, the capacity of a pipe was the same as the system page size 
(e.g., 4096 bytes on i386). Since Linux 2.6.11, the pipe capacity is 65536 bytes.
Simplify things here : allow maximum size of 4096 for now */
#define MAX_DATA_SIZE   4096

int MblSdbusPipe_create(MblSdbusPipe *pipe_object) 
{
    int r;

    memset (pipe_object, 0, sizeof(pipe_object));
    r = pipe2(pipe_object->pipefd, O_NONBLOCK);
    if (r != 0){
        return r;
    }

    // The first index is used for reading, polled for incoming input
    pipe_object->pollfd[READ].fd = pipe_object->pipefd[READ];
    pipe_object->pollfd[READ].events = POLLIN;
    pipe_object->pollfd[READ].revents = 0;

    // The second index is used for writing, polled to check if writing is possible
    pipe_object->pollfd[WRITE].fd = pipe_object->pipefd[WRITE];
    pipe_object->pollfd[WRITE].events = POLLOUT;
    pipe_object->pollfd[WRITE].revents = 0;

    return 0;
}

int MblSdbusPipe_destroy(MblSdbusPipe *pipe_object) 
{
    int r;
    
    // TODO : we need to make sure no one is reading/writing to the pipe
    // usually each side closes its egde, but here we might make easier assumptions    
    r = close(pipe_object->pipefd[0]);
    if (r != 0){
        return r;
    }    

    r = close(pipe_object->pipefd[1]);
    if (r != 0){
        return r;
    }    

    memset (pipe_object, 0, sizeof(pipe_object));
    return 0;
}


int MblSdbusPipe_data_send(MblSdbusPipe *pipe_object , u_int8_t *data, uint32_t data_size) 
{
    int r;
    u_int8_t *data_ = NULL;

    if (data_size > MAX_DATA_SIZE){
        return -1;
    }

    // lets be sure that pipe is ready for write, we do not wait and do not retry
    // since this pipe is used only to transfer pointers - being full shows we have 
    // critical issue    
    r = poll(&pipe_object->pollfd[WRITE], 1, MAX_TIME_TO_POLL_MILLISEC);
    if (r == 0){
        // timeout!
        return -1;
    }
    if (r < 0){
        //some error!
        return r;
    }
    if (pipe_object->pollfd[WRITE].revents & POLLOUT) {
        //can write - write the allocated pointer
        r = write(pipe_object->pollfd[WRITE].fd, data, data_size);
        if (r <= 0){
            //nothing written or error!
            return r;
        }
        if (r != sizeof(data)){
            return -1;
        }
    }
    else {
        //unexpected event!
        return -1;
    }

    return 0;
}

// receiver must free message
int MblSdbusPipe_data_receive(MblSdbusPipe *pipe_object , u_int8_t **data)
{
    int r;
    uint8_t *data_;

    r = poll(&pipe_object->pollfd[READ], 1, MAX_TIME_TO_POLL_MILLISEC);
    if (r == 0){
        // timeout!
        return -1;
    }
    if (r < 0){
        //some error!
        return r;
    }
    if (pipe_object->pollfd[READ].revents & POLLIN) {
        //can read - get the allocated pointer
        r = read(pipe_object->pollfd[READ].fd, &data, sizeof(uint8_t*));
        if (r <= 0){
            //nothing read or error!
           return r;
        }
        if (r != sizeof(data)){
            return -1;
        }
    }
    else {
        //unexpected event!
        return -1;
    }

    *data = data_;
    return 0;
}