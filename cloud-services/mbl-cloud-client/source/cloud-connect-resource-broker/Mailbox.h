/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef _Mailbox_h_
#define _Mailbox_h_

#include "MblError.h"
#include "MailboxMsg.h"

#include <poll.h>

#include <inttypes.h>
#include <string>

namespace mbl {

struct MailboxMsg;

/**
 * @brief - One way mailbox implemented using POSIX pipes. It is NOT thread safe - we assume that
 * there is a single producer and a single consumer. This mailbox is implemented for inter-thread
 * communication and NOT inter process communication. Since it is used for multi-thread 
 * communication, the messages sent on the pipe using pointers. So in order to send a message, 
 * the producer 'pays' a system call write() and writes sizeof(uintptr_t) bytes on to the pipe.
 * In order to avoid unlimited blocking inside the mailbox, a polling mechanism is used. 
 * sending / receiving a message to mail box will be blocked by a maximum upper bound of 
 * Mailbox::MAILBOX_MAX_POLLING_TIME_MILLISECONDS.
 * The actual allocation/deallocation of messages is done inside the mailbox, and transparent to 
 * user.
 * The mailbox can be attached to an sd-event event loop object by adding its READ fd as an IO 
 * event source.
 * For all mailbox operations beside init(), it is assumed (and asserted) that mailbox is already 
 * initialized
 */
class Mailbox
{
public:    
    // maximum time to wait inside the mailbox
    static constexpr int MAILBOX_MAX_POLLING_TIME_MILLISECONDS = 30;

    /**
     * @brief Construct a new Mailbox object
     * 
     * @param name - name should be given in order to identify the mailbox 
     * (while debugging). Use get_name() to fetch the mailbox name
     */
    Mailbox(const char* name);

    /**
     * @brief Initialize the mailbox - create pipe, assign polling 
     * information and set a protection flag.
     * 
     * @return MblError - return MblError - Error::None for success, therwise the failure reason
     */
    MblError  init();

    //TODO make sure producer doesnt touch the mailbox (use initialzer thread id) 
    // (using CCRB API - not not use any mutexes!)
    /**
     * @brief deinitialize mailbox, removes and clear any pending events if exist.
     * both pipes are closed at the same time - we implement other mechanisms to prevent the 
     * producer to access the pipe after/while deinit executed
     * 
     * @return MblError - return MblError - Error::None for success, therwise the failure reason
     */
    MblError  deinit();

    /**
     * @brief send a message to CCRB thread event loop by an external thread.In the normal 
     * operation, the callling thread shouldn't be blocked at all, no competitors and only a 
     * pointer is written. The thread will poll on the WRITE pipe side up to timeout_milliseconds 
     * time and exit with error if timeout occurred.
     * 
     * @param msg - a message to be sent
     * @param timeout_milliseconds - (optional) maximum time to wait for sending the message. 
     * recommended to be set to MAILBOX_MAX_POLLING_TIME_MILLISECONDS.
     * @return MblError - return MblError - Error::None for success, therwise the failure reason
     */
    MblError  send_msg( MailboxMsg &msg_to_send,
                        int timeout_milliseconds = Mailbox::MAILBOX_MAX_POLLING_TIME_MILLISECONDS);

    /**
     * @brief receive a message from external thread.
     * 
     * @param msg 
     * @param timeout_milliseconds - maximum time to wait for receiving the message. recommended to 
     * be set to MAILBOX_MAX_POLLING_TIME_MILLISECONDS.
     * @return MblError - return MblError - Error::None for success, therwise the failure reason
     */

    /**
     * @brief 
     * 
     * @param timeout_milliseconds - (optional) maximum time to wait for receiving the message. 
     * recommended to be set to MAILBOX_MAX_POLLING_TIME_MILLISECONDS.
     * @return std::pair<MblError, MailboxMsg> - a pair where the first element Error::None for 
     * success, therwise the failure reason.
     * If the first element is Error::None, user may access the second element which is the 
     * message received. most compilers will optimize (move and not copy). If first element is 
     * not success - user should ignore the second element.
     */
    std::pair<MblError, MailboxMsg>  receive_msg(
        int timeout_milliseconds = Mailbox::MAILBOX_MAX_POLLING_TIME_MILLISECONDS);

    /**
     * @brief Get the pipefd read object - this is used in order to attach an incoming mailbox
     * to the sd-event loop object.
     * @return int - return the fd.
     */
    inline int       get_pipefd_read() { return pipefds_[READ]; }

    /**
     * @brief Get the name of the mail box assigned on ctor
     * 
     * @return const char* - the name
     */
    inline const char* get_name() { return name_.c_str(); }
private:

    static constexpr int READ = 0;
    static constexpr int WRITE = 1;
    static constexpr int DBUS_MAILBOX_PROTECTION_FLAG = 0xF0F0F0F0;
    
    /**
     * @brief - implements init()
     * 
     * @return MblError - Error::None for success running the loop, otherwise the failure reason
     */
    MblError do_init();

    /**
     * @brief poll on pollfds_ index poll_fd_index for timeout_milliseconds
     * 
     * @return - Error::None for success , otherwise the failure reason 
     */
    MblError do_poll(int poll_fd_index, int timeout_milliseconds);

    std::string name_;

    //use to protect against corruption and mark that mailbox is initialized
    int    protection_flag_ = DBUS_MAILBOX_PROTECTION_FLAG;

    // Store read (0) and write (1) file descriptors for the pipe
    int           pipefds_[2];

    // Store polling file descriptors on the pipe
    struct pollfd pollfds_[2];
};


}//namespace mbl

#endif // _Mailbox_h_
