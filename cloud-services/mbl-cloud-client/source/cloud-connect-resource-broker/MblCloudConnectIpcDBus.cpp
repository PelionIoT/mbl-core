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

#include "MblCloudConnectIpcDBus.h"

#include "mbed-trace/mbed_trace.h"
#include <pthread.h>
#include <cassert.h>
#include <unistd.h>

#define TRACE_GROUP "CCRB-IPCDBUS"

namespace mbl {

MblCloudConnectIpcDBus::MblCloudConnectIpcDBus()
{
    tr_info("MblCloudConnectIpcDBus::MblCloudConnectIpcDBus");

	// constructor runs on IPC thread context
    // store thread ID
	ipc_thread_id = pthread_self();
}

MblCloudConnectIpcDBus::~MblCloudConnectIpcDBus()
{
    tr_debug("MblCloudConnectIpcDBus::~MblCloudConnectIpcDBus");
}

MblError MblCloudConnectIpcDBus::Init()
{
    tr_debug("MblCloudConnectIpcDBus::Init");
    return Error::None;
}

MblError MblCloudConnectIpcDBus::Run()
{
    tr_debug("MblCloudConnectIpcDBus::Run");

    while not should finish
	{
    	sleep
		print
	}

    return Error::None;
}

MblError MblCloudConnectIpcDBus::ThreadJoin(void** args)
{
    tr_info("MblCloudConnectIpcDBus::ThreadJoin");

    const int thread_join_err = pthread_join(ipc_thread_id, args);
    if(0 != thread_join_err)
    {
        // thread joining failed, print errno value and exit
        const int thread_join_errno = errno;

        // handle linux error
        std::fprintf(
			stderr,
			"Thread joining failed (%s)!\n",
			strerror(thread_join_errno));

		tr_err(
            "Thread joining failed (%s)!\n",
            strerror(thread_join_errno));

        return Error::ThreadJoiningFailed;
    }

    return Error::None;
}

MblError MblCloudConnectIpcDBus::ThreadFinish()
{
    set flag to FINISH!;
	return Error::None;
}

} // namespace mbl

