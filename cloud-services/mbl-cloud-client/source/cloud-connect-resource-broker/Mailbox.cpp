/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define TRACE_GROUP "ccrb-mailbox"

#include "CloudConnectTrace.h"
#include "Mailbox.h"
#include "MailboxMsg.h"
#include "MblError.h"
#include "CloudConnectTypes.h"

#include <cassert>
#include <stdlib.h>
#include <fcntl.h>              /* Obtain O_* constant definitions */
#include <unistd.h>
#include <errno.h> 
#include <memory>

namespace mbl {

Mailbox::Mailbox(const char* name) : 
    name_(name),
    protection_flag_(DBUS_MAILBOX_PROTECTION_FLAG),
    pipefds_{},
    pollfds_{}
{
    TR_DEBUG("Enter");
    assert(name);
}

MblError Mailbox::init()
{
    TR_DEBUG("Enter");
    // call do_init in order to deinit on failure
    MblError status = do_init();
    TR_ERR("do_init failed, call deinit");
    if (status != MblError::None){
        deinit();
    }
    return status;
}

MblError Mailbox::do_init()
{
    TR_DEBUG("Enter");

    // open unnamed pipe with flag O_NONBLOCK
    // This flag instructs the kernel to release the thread immediately
    // in case that pipe blocks. It is stringly recommended by sd-event to attach IO sources which
    // are set with that flag
    int r = pipe2(pipefds_, O_NONBLOCK);
    if (r != 0){
        TR_ERR("pipe2 failed with error r=%d (%s) - returning %s",
            r, strerror(r), MblError_to_str(MblError::DBA_MailBoxSystemCallFailure));
        return MblError::DBA_MailBoxSystemCallFailure;
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
    
    TR_INFO("Initialized new mailbox=%s pipefds_[READ]=%d pipefds_[WRITE]=%d", 
        get_name(), pipefds_[READ], pipefds_[WRITE]);
    return MblError::None;
}

const char* Mailbox::get_name() 
{
    TR_DEBUG("Enter");
    return name_.c_str(); 
}

MblError Mailbox::deinit() 
{
    OneSetMblError status;
    TR_DEBUG("Enter");
    assert(DBUS_MAILBOX_PROTECTION_FLAG == protection_flag_);

    // TODO : VERY IMPORTANT - add code here which will empty the mailbox and free all pointers.
    // TODO : we need to make sure no one is reading/writing to the pipe - implement this by 
    // external API
    // usually each side closes its edge, but here we might make easier assumptions    
    int r = close(pipefds_[READ]);
    if (r != 0){
        //there is not much you can do about errors on close()
        status.set(MblError::DBA_MailBoxSystemCallFailure);
        TR_ERR("close(pipefds_[READ]) failed with error r=%d (%s) - returning %s",
            r, strerror(r), status.get_status_str());        
        //continue - best effort
    }    
    r = close(pipefds_[WRITE]);
    if (r != 0){
        //there is not much you can do about errors on close()
        status.set(MblError::DBA_MailBoxSystemCallFailure);
        TR_ERR("close(pipefds_[WRITE]) failed with error r=%d (%s) - returning %s",
            r, strerror(r), status.get_status_str());        
        //continue - best effort
    }
    protection_flag_ = 0;
    if (status.get() == MblError::None){
        TR_INFO("SUCCESS!");
    }
    return status.get();
}

MblError Mailbox::do_poll(int poll_fd_index, int timeout_milliseconds)
{
    assert((poll_fd_index == WRITE) || (poll_fd_index == READ));
    
    // lets be sure that pipe is ready for write/read, we do not wait and do not retry
    // since this pipe is used only to transfer pointers - being full shows we have 
    // critical issue    
    int r = poll(&pollfds_[poll_fd_index], 1, timeout_milliseconds);
    if (r == 0){
        // timeout!
        TR_ERR("poll failed with timeout (pipe full?)! returning %s",
            MblError_to_str(MblError::DBA_MailBoxPollTimeout));
        return MblError::DBA_MailBoxPollTimeout;
    }
    if (r < 0){
        //some ther error (no timeout)!
        TR_ERR("poll failed with error r=%d (%s) - returning %s",
            r, strerror(r), MblError_to_str(MblError::DBA_MailBoxSystemCallFailure));
            return MblError::DBA_MailBoxSystemCallFailure;
    }

    return MblError::None;
}

MblError  Mailbox::send_msg(MailboxMsg &in_msg, int timeout_milliseconds)
{
    assert(DBUS_MAILBOX_PROTECTION_FLAG == protection_flag_);
    TR_DEBUG("Enter");
      
    MblError status = do_poll(WRITE, timeout_milliseconds);
    if (MblError::None != status){
        TR_ERR("do_poll failed with error %s", MblError_to_str(status));
        return status;
    }
   
    // Free for write! try..
    if (pollfds_[WRITE].revents & POLLOUT) {        

        // can now write - create a unique_ptr holding a new allocated message, and 
        // write the message pointer address. keep address for debug
        std::unique_ptr<MailboxMsg> local_msg(new MailboxMsg(in_msg));
        auto ptr_to_write = local_msg.get();    //write lvalue
        ssize_t bytes_written = write(pipefds_[WRITE], &ptr_to_write, sizeof(MailboxMsg*));
        if (bytes_written == 0){
            //nothing written!
            TR_ERR("write failed - zero bytes written! - returning %s",
                MblError_to_str(MblError::DBA_MailBoxSystemCallFailure));
            return MblError::DBA_MailBoxSystemCallFailure;
        }
        if (bytes_written < 0){
            // some other error!
            TR_ERR("write failed (bytes_written=%zd < 0) - returning %s",
                bytes_written, MblError_to_str(MblError::DBA_MailBoxSystemCallFailure));
                return MblError::DBA_MailBoxSystemCallFailure;
        }
        if (bytes_written != sizeof(MailboxMsg*)){
            TR_ERR("write failed - unexpected number of bytes written!"
                " (bytes_written=%zd) - returning %s", bytes_written, 
                MblError_to_str(MblError::DBA_MailBoxSystemCallFailure));
            return MblError::DBA_MailBoxSystemCallFailure;
        }
        //write was successful!, write to log and release the unique ptr . 
        //be carefull not to dereference the unique_ptr, it might be free!

        TR_INFO("Message sent via %s mailbox. address=%p, sequence_num_=%" PRIu64
                " payload_len_=%zu type=%s",
            get_name(),
            ptr_to_write,
            in_msg.sequence_num_,
            in_msg.payload_len_,
            in_msg.MsgType_to_str());
        local_msg.release();
   }
   else {
        //unexpected event!
        TR_ERR("Unexpected revents after polling succeeded on pollfds_[WRITE], expected POLLOUT"
            " but pollfds_[WRITE].revents=%#06x - returning %s",
            pollfds_[WRITE].revents,
            MblError_to_str(MblError::DBA_MailBoxSystemCallFailure));
        return MblError::DBA_MailBoxSystemCallFailure;
    }   

    return MblError::None;
}

std::pair<MblError, MailboxMsg>  Mailbox::receive_msg(int timeout_milliseconds)
{
    TR_DEBUG("Enter");
    MailboxMsg *msg_ = nullptr;
    assert(DBUS_MAILBOX_PROTECTION_FLAG == protection_flag_);
    std::pair<MblError, MailboxMsg> ret_pair(MblError::None, MailboxMsg());

    ret_pair.first = do_poll(READ, timeout_milliseconds);
    if (MblError::None != ret_pair.first){
        TR_ERR("do_poll failed with error %s", MblError_to_str(ret_pair.first));
        return ret_pair;
    }

    // Free for read! try..
    if (pollfds_[READ].revents & POLLIN) {
        //can now read- read the message pointer address
        ssize_t bytes_read = read(pipefds_[READ], &msg_, sizeof(MailboxMsg*));
        if (bytes_read == 0){
            //nothing read!
            TR_ERR("read failed - zero bytes read! - returning %s",
                MblError_to_str(MblError::DBA_MailBoxSystemCallFailure));
            ret_pair.first = MblError::DBA_MailBoxSystemCallFailure;
            return ret_pair;
        }
        if (bytes_read < 0){
            // some other error!
            TR_ERR("read failed (bytes_read=%zd < 0) - returning %s",
                bytes_read, MblError_to_str(MblError::DBA_MailBoxSystemCallFailure));
            ret_pair.first = MblError::DBA_MailBoxSystemCallFailure;
            return ret_pair;
        }
        if (bytes_read != sizeof(MailboxMsg*)){
            TR_ERR("read failed - unexpected number of bytes read!"
                " (bytes_read=%zd) - returning %s", bytes_read, 
                MblError_to_str(MblError::DBA_MailBoxSystemCallFailure));
            ret_pair.first = MblError::DBA_MailBoxSystemCallFailure;
            return ret_pair;
        }       
        if (nullptr == msg_){
            TR_ERR("read failed - msg_ is nullptr! returning %s", 
                MblError_to_str(MblError::DBA_MailBoxSystemCallFailure));
            ret_pair.first = MblError::DBA_MailBoxSystemCallFailure;
            return ret_pair;
        }
    }
    else {
        TR_ERR("Unexpected revents after polling succeeded on pollfds_[READ], expected POLLIN"
            " but pollfds_[READ].revents=%#06x - returning %s",
            pollfds_[READ].revents,
            MblError_to_str(MblError::DBA_MailBoxSystemCallFailure));
        ret_pair.first = MblError::DBA_MailBoxSystemCallFailure;
        return ret_pair;
    }
    assert(MailboxMsg::MSG_PROTECTION_FIELD == msg_->protection_field_);

    TR_INFO("Message received via %s mailbox. address=%p, sequence_num=%" PRIu64
            " payload_len_=%zu type=%s",
        get_name(),
        msg_,
        msg_->sequence_num_,
        msg_->payload_len_,
        msg_->MsgType_to_str());

    // Assign pointer to reference and delete it
    ret_pair.second = *msg_;
    delete(msg_);

    return ret_pair;
}


int Mailbox::get_pipefd_read()
{    
    TR_DEBUG("Enter");
    assert(DBUS_MAILBOX_PROTECTION_FLAG == protection_flag_);
    return pipefds_[READ];
}

}//namespace mbl
