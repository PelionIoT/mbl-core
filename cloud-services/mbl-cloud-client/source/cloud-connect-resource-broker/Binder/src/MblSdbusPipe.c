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


int MblSdbusPipe_msg_send(MblSdbusPipe *pipe_object , MblSdbusPipeMsg *_msg) 
{
    int r;
    MblSdbusPipeMsg *msg = NULL;

    if (_msg->type >= PIPE_MSG_TYPE_LAST){
        return -1;
    }
    msg = (MblSdbusPipeMsg*)malloc(sizeof(MblSdbusPipeMsg));
    if (NULL == msg){
        return -1;
    }
    memcpy(msg, _msg, sizeof(MblSdbusPipeMsg));
    
    r = poll(&pipe_object->pollfd[WRITE], 1, MAX_TIME_TO_POLL_MILLISEC);
    if (r == 0){
        // timeout!
        r = -1;
        goto on_failure;
    }
    if (r < 0){
        //some error!
        goto on_failure;
    }
    if (pipe_object->pollfd[WRITE].revents & POLLOUT) {
        //can write - write the allocated pointer
        r = write(pipe_object->pollfd[WRITE].fd, &msg, sizeof(MblSdbusPipeMsg*));
        if (r <= 0){
            //nothing written or error!
            goto on_failure;
        }
        if (r != sizeof(msg)){
            r = -1;
            goto on_failure;
        }
    }
    else {
        //unexpected event!
        r = -1;
        goto on_failure;
    }

    return 0;

on_failure:
    if (NULL != msg){
        free(msg);
    }
    return r;
}

// receiver must free message
int MblSdbusPipe_msg_receive(MblSdbusPipe *pipe_object , MblSdbusPipeMsg **_msg)
{
    int r;
    MblSdbusPipeMsg *msg;

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
        r = read(pipe_object->pollfd[READ].fd, &msg, sizeof(MblSdbusPipeMsg*));
        if (r <= 0){
            //nothing read or error!
           return r;
        }
        if (r != sizeof(msg)){
            return -1;
        }
    }
    else {
        //unexpected event!
        return -1;
    }

    *_msg = msg;
    return 0;
}