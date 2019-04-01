/*
 * Copyright (c) 2017 ARM Ltd.
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

#include "application_init.h"
#include "log.h"
#include "cloud-connect-resource-broker/ResourceBroker.h"
// #include "signals.h"

#include "mbed-trace/mbed_trace.h"

#include <cerrno>
#include <cstdio>
#include <unistd.h>

#define TRACE_GROUP "main"

using namespace mbl;

// static int aaa (int signal)
// {
//     TR_DEBUG_ENTER;
//     sigset_t mask;
//     sigset_t orig_mask;
//     sigemptyset (&mask);
// 	sigaddset (&mask, signal);
 
//     //int r = sigprocmask(SIG_BLOCK, &mask, &orig_mask);
//     int r = pthread_sigmask(SIG_BLOCK, &mask, &orig_mask); 
// 	if (r < 0) {
// 		TR_ERR("sigprocmask failed with error: %d - %s",
//             r,
//             strerror(-r)
//         );
// 	}
//     return r;
// }

int main()
{
    const int daemon_err = daemon(0 /* nochdir */, 0 /* noclose */);
    const int daemon_errno = errno;
    if (daemon_err != 0) {
        std::fprintf(
            stderr,
            "Daemonization failed (%s), exiting application!\n",
            strerror(daemon_errno));

        // If daemon() got far enough, stderr might be /dev/null so make sure
        // we write to the log as well
        tr_err(
            "Daemonization failed (%s), exiting application!\n",
            strerror(daemon_errno));
        return 1;
    }

    const MblError log_err = log_init(nullptr);
    if (log_err != Error::None) {
        std::fprintf(
            stderr,
            "Log initialization failed (%s), exiting application!\n",
            MblError_to_str(log_err));
        return 1;
    }


    // aaa(SIGTERM);
    // aaa(SIGUSR1);
    // aaa(SIGUSR2);
    
    // const MblError sig_err = signals_init();
    // if (sig_err != Error::None) {
    //     tr_err("Signal handler initialization failed (%s), exiting application!",
    //         MblError_to_str(sig_err));
    //     return 1;
    // }


    if (!application_init()) {
        tr_err("Cloud Client library initialization failed, exiting application.");
        return 1;
    }

    const MblError run_err = ResourceBroker::main();

    tr_info("Exiting application");
    return (run_err == Error::ShutdownRequested)? 0 : 1;
}
