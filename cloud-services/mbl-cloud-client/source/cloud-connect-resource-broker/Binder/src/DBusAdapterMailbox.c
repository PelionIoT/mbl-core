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
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h> 


#include "DBusAdapterMsg.h"
#include "DBusAdapterMailbox.h"


#define READ                        0
#define WRITE                       1
#define DBUS_MAILBOX_PROTECTION_FLAG    0xF0F0F0F0

int DBusAdapterMailbox_alloc(DBusAdapterMailbox *mailbox) 
{
    // TODO - consider refacto all 'r' to 'retval'
    int r;

    assert(NULL != mailbox);
    assert(DBUS_MAILBOX_PROTECTION_FLAG != mailbox->protection_flag);

    memset (mailbox, 0, sizeof(mailbox));
    r = pipe2(mailbox->pipefd, O_NONBLOCK);
    if (r != 0){
        return r;
    }

    // The first index is used for reading, polled for incoming input
    mailbox->pollfd[READ].fd = mailbox->pipefd[READ];
    mailbox->pollfd[READ].events = POLLIN;
    mailbox->pollfd[READ].revents = 0;

    // The second index is used for writing, polled to check if writing is possible
    mailbox->pollfd[WRITE].fd = mailbox->pipefd[WRITE];
    mailbox->pollfd[WRITE].events = POLLOUT;
    mailbox->pollfd[WRITE].revents = 0;
    mailbox->protection_flag = DBUS_MAILBOX_PROTECTION_FLAG;
    return 0;
}

int DBusAdapterMailbox_free(DBusAdapterMailbox *mailbox) 
{
    int r;
    
    // TODO : we need to make sure no one is reading/writing to the pipe
    // usually each side closes its edge, but here we might make easier assumptions    
    r = close(mailbox->pipefd[0]);
    if (r != 0){
        return r;
    }    

    r = close(mailbox->pipefd[1]);
    if (r != 0){
        return r;
    }    

    memset (mailbox, 0, sizeof(mailbox));
    return 0;
}

int DBusAdapterMailbox_send(DBusAdapterMailbox *mailbox, struct DBusAdapterMsg *msg, int timeout_milliseconds)
{
    int r;

    if (NULL == msg){
        return -1;
    }
    if (msg->payload_len > DBUS_MAX_MSG_PAYLOAD_SIZE){
        return -1;
    }
    if (msg->payload_len == 0){
        return -1;
    }
    
    // lets be sure that pipe is ready for write, we do not wait and do not retry
    // since this pipe is used only to transfer pointers - being full shows we have 
    // critical issue    
    r = poll(&mailbox->pollfd[WRITE], 1, timeout_milliseconds);
    if (r == 0){
        // timeout!
        return -1;
    }
    if (r < 0){
        //some error!
        printf("%d\n", errno);
        return r;
    }
    if (mailbox->pollfd[WRITE].revents & POLLOUT) {
        //can write - write a full message type, even if some unneeded data is written at the end
        msg->_sequence_num = mailbox->_sequence_num++;
            r = write(mailbox->pipefd[WRITE], msg, sizeof(struct DBusAdapterMsg));
        if (r <= 0){
            //nothing written or error!
            return r;
        }
        if (r != sizeof(struct DBusAdapterMsg)){
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
int DBusAdapterMailbox_receive(DBusAdapterMailbox *mailbox, struct DBusAdapterMsg *msg, int timeout_milliseconds)
{
    int r;

    if (NULL == msg){
        return -1;
    }

    r = poll(&mailbox->pollfd[READ], 1, timeout_milliseconds);
    if (r == 0){
        // timeout!
        return -1;
    }
    if (r < 0){
        //some error!
        return r;
    }
    if (mailbox->pollfd[READ].revents & POLLIN) {
        //can read - read maximum bytes
        r = read(mailbox->pipefd[READ], msg, sizeof(struct DBusAdapterMsg));
        if (r <= 0){
            //nothing read or error!
           return r;
        }
        if (r != sizeof(struct DBusAdapterMsg)){
            return -1;
        }
    }
    else {
        //unexpected event!
        return -1;
    }

    return 0;
}
