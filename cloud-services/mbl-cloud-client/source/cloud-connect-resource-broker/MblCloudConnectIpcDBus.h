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

/**
 * @brief This class provides an implementation for D-Bus IPC mechanism.
 * Implements MblCloudConnectIpcInterface interface. 
 */
class MblCloudConnectIpcDBus: public MblCloudConnectIpcInterface {

public:

    MblCloudConnectIpcDBus();
    ~MblCloudConnectIpcDBus() override;

    // Implementation of MblCloudConnectIpcInterface::init()
    MblError init() override;

    // Implementation of MblCloudConnectIpcInterface::de_init()
    MblError de_init() override;

    // Implementation of MblCloudConnectIpcInterface::run()
    MblError run() override;

    // Implementation of MblCloudConnectIpcInterface::stop()
    MblError stop() override;

private:
   
    // Temporary solution for exiting from simulated event-loop that should be removed after we introduce real sd-bus event-loop.
    // Now we just use this boolean flag, that signals, that the thread should exit from simulated event-loop.
    // In future we shall replace this flag with real mechanism, that will allow exiting from real sd-bus event-loop.
    volatile bool exit_loop_;
};

} // namespace mbl

#endif // MblCloudConnectIpcDBus_h_
