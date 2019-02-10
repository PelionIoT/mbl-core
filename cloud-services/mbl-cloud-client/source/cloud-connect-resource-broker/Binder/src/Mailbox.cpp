/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>              /* Obtain O_* constant definitions */
#include <unistd.h>
#include <errno.h> 
#include <memory>

#include "Mailbox.h"
#include "MailboxMsg.h"
#include "MblError.h"

namespace mbl {

MblError Mailbox::init()
{
    MblError status = do_init();
    if (status != MblError::None){
        deinit();
    }
    return status;
}

MblError Mailbox::do_init()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
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

MblError Mailbox::deinit() 
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    int r;        
    assert(DBUS_MAILBOX_PROTECTION_FLAG == protection_flag_);

    // TODO : VERY IMPORTANT - add code here which will empty the mailbox and free all pointers.

    // TODO : we need to make sure no one is reading/writing to the pipe
    // usually each side closes its edge, but here we might make easier assumptions    
    r = close(pipefds_[0]);
    if (r != 0){
        //there is not much you can do about errors on close()
        return MblError::DBusErr_Temporary;
    }    
    r = close(pipefds_[1]);
    if (r != 0){
        //there is not much you can do about errors on close()
        return MblError::DBusErr_Temporary;
    }
    sequence_num_ = 0;
    return MblError::None;
}

MblError Mailbox::send_msg(MailboxMsg &msg, int timeout_milliseconds)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    int r;    
    // FIXME : We can avoid allocation at all. create vector of smart pointers with atomic bool which shows if
    // memory is used or not. this requires extra work. TBD.
    MailboxMsg *msg_ = new MailboxMsg(msg);

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
        msg_->_sequence_num = sequence_num_++;
        r = write(pipefds_[WRITE], &msg_, sizeof(MailboxMsg*));
        if (r <= 0){
            //nothing written or error!
            return MblError::DBusErr_Temporary;
        }
        if (r != sizeof(MailboxMsg*)){
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
MblError Mailbox::receive_msg(MailboxMsg &msg, int timeout_milliseconds)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    int r;
    MailboxMsg *msg_;
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
        r = read(pipefds_[READ], &msg_, sizeof(MailboxMsg*));
        if (r <= 0){
            //nothing read or error!
           return MblError::DBusErr_Temporary;
        }
        if (r != sizeof(MailboxMsg*)){
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
    delete(msg_);
    return MblError::None;
}


int Mailbox::get_pipefd_read()
{
    return pipefds_[READ];
}

}//namespace mbl
