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


#ifndef _DBusAdapterMailbox_h_
#define _DBusAdapterMailbox_h_

#include <poll.h>
#include <stdint.h>

#include "MblError.h"

namespace mbl {

struct DBusAdapterMsg;
#define DBUS_MAILBOX_PROTECTION_FLAG    0xF0F0F0F0

class DBusAdapterMailbox
{
    public:    
    // TODO : fix all name of this module to DBusAdapterMailbox
    MblError init();
    MblError deinit();
    MblError send_msg(DBusAdapterMsg &msg, int timeout_milliseconds);
    MblError receive_msg(DBusAdapterMsg &msg, int timeout_milliseconds);

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