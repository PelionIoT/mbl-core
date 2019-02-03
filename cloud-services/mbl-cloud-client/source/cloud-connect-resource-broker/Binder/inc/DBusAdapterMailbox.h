/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef _DBusAdapterMailbox_h_
#define _DBusAdapterMailbox_h_

#include <poll.h>
#include <stdint.h>

#include "MblError.h"

namespace mbl {


#define DBUS_MAILBOX_PROTECTION_FLAG        0xF0F0F0F0
#define DBUS_MAILBOX_TIMEOUT_MILLISECONDS   1000

struct DBusMailboxMsg;
class DBusAdapter;
class DBusAdapterMailbox
{
    public:    

    // TODO : fix all name of this module to DBusAdapterMailbox
    MblError  init();
    MblError  deinit();
    MblError  send_msg(DBusMailboxMsg &msg, int timeout_milliseconds);
    MblError  receive_msg(DBusMailboxMsg &msg, int timeout_milliseconds);
    int       get_pipefd_read();

    private:
    static const int READ = 0;
    static const int WRITE = 1;

    uint32_t      protection_flag_ = DBUS_MAILBOX_PROTECTION_FLAG;
    uint64_t      sequence_num_ = 0; // starting from 0 and incremented
    int           pipefds_[2];    // Store read (0) and write (1) file descriptors for the pipe
    struct pollfd pollfds_[2];    // Store polling file descriptors on the pipe
};


}//namespace mbl
#endif // _DBusAdapterMailbox_h_