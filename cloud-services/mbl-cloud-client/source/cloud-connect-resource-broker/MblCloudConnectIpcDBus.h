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


#ifndef MblCloudConnectIpcDBus_h_
#define MblCloudConnectIpcDBus_h_

#include "MblCloudConnectIpcInterface.h"
#include <pthread.h>

namespace mbl {

/*! \file MblCloudConnectIpcDBus.h
 *  \brief MblCloudConnectIpcDBus.
 *  This class provides an implementation for D-Bus IPC mechanism
 */
class MblCloudConnectIpcDBus: public MblCloudConnectIpcInterface {

public:

    MblCloudConnectIpcDBus();
    ~MblCloudConnectIpcDBus() override;

    // Implementation of init()
    MblError init() override;

    // Implementation of run()
    MblError run() override;

    // Implementation of thread_join()
    MblError thread_join(void **args) override;

    // Implementation of thread_finish()
    MblError thread_finish() override;

private:
    pthread_t ipc_thread_id;

    // temporary not thread safe solution (TO BE REMOVED SOONLY)
    volatile bool should_finish_asap;
};

} // namespace mbl

#endif // MblCloudConnectIpcDBus_h_
