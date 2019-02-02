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

//#define _GNU_SOURCE             /* See feature_test_macros(7) */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>              /* Obtain O_* constant definitions */
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h> 

#include "DBusAdapterMailbox.h"
#include "DBusAdapterMsg.h"
#include "MblError.h"

namespace mbl {
MblError DBusAdapterMailbox::init() 
{
    // TODO - consider refacto all 'r' to 'retval'
    int r;

    // open unnamed pipe with flag O_NONBLOCK
    // This flag insturcts the kernel to release the thread immidiatly
    // in case that pipe blocks 
    r = pipe2(pipefds_, O_NONBLOCK);
    if (r != 0){
        return MblError::DBusErr_Temporary;
    }

    // The first index is used for reading, polled for incoming input
    pollfds_[READ].fd = pipefds_[READ];
    pollfds_[READ].events = POLLIN;
    pollfds_[READ].revents = 0;

    // The second index is used for writing, polled to check if writing is possible
    pollfds_[WRITE].fd = pipefds_[WRITE];
    pollfds_[WRITE].events = POLLOUT;
    pollfds_[WRITE].revents = 0;

    protection_flag_ = DBUS_MAILBOX_PROTECTION_FLAG;
    
    return MblError::None;
}

MblError DBusAdapterMailbox::deinit() 
{
    int r;
    
    assert(DBUS_MAILBOX_PROTECTION_FLAG == protection_flag_);
    // TODO : we need to make sure no one is reading/writing to the pipe
    // usually each side closes its edge, but here we might make easier assumptions    
    r = close(pipefds_[0]);
    if (r != 0){
        return MblError::DBusErr_Temporary;
    }    
    r = close(pipefds_[1]);
    if (r != 0){
        return MblError::DBusErr_Temporary;
    }    
    return MblError::None;
}

MblError DBusAdapterMailbox::send_msg(DBusAdapterMsg &msg, int timeout_milliseconds)
{
    int r;
    DBusAdapterMsg *msg_ = new DBusAdapterMsg(msg);

    assert(DBUS_MAILBOX_PROTECTION_FLAG == protection_flag_);
    
    // lets be sure that pipe is ready for write, we do not wait and do not retry
    // since this pipe is used only to transfer pointers - being full shows we have 
    // critical issue    
    r = poll(&pollfds_[WRITE], 1, timeout_milliseconds);
    if (r == 0){
        // timeout!
        return MblError::DBusErr_Temporary;
    }
    if (r < 0){
        //some error!
        printf("%d\n", errno);
        return MblError::DBusErr_Temporary;
    }
    if (pollfds_[WRITE].revents & POLLOUT) {
        //can write - write the message pointer address
        msg_->header.sequence_num = sequence_num_++;
        r = write(pipefds_[WRITE], &msg_, sizeof(DBusAdapterMsg*));
        if (r <= 0){
            //nothing written or error!
            return MblError::DBusErr_Temporary;
        }
        if (r != sizeof(DBusAdapterMsg*)){
           return MblError::DBusErr_Temporary;
       }
   }
   else {
       //unexpected event!
       return MblError::DBusErr_Temporary;
   }

    return MblError::None;
}

// receiver must free message
MblError DBusAdapterMailbox::receive_msg(DBusAdapterMsg &msg, int timeout_milliseconds)
{
    int r;
    DBusAdapterMsg *msg_;
    assert(DBUS_MAILBOX_PROTECTION_FLAG == protection_flag_);

    r = poll(&pollfds_[READ], 1, timeout_milliseconds);
    if (r == 0){
        // timeout!
        return MblError::DBusErr_Temporary;
    }
    if (r < 0){
        //some error!
        return MblError::DBusErr_Temporary;
    }
    if (pollfds_[READ].revents & POLLIN) {
        r = read(pipefds_[READ], &msg_, sizeof(DBusAdapterMsg*));
        if (r <= 0){
            //nothing read or error!
           return MblError::DBusErr_Temporary;
        }
        if (r != sizeof(DBusAdapterMsg*)){
            return MblError::DBusErr_Temporary;
        }
        if (nullptr == msg_){
            return MblError::DBusErr_Temporary;
        }
    }
    else {
        //unexpected event!
        return MblError::DBusErr_Temporary;
    }
    msg = *msg_;
    free(msg_);
    return MblError::None;
}

}//namespace mbl