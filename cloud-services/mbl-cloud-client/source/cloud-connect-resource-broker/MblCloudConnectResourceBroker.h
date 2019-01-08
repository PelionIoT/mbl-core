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


#ifndef MblCloudConnectResourceBroker_h_
#define MblCloudConnectResourceBroker_h_

#include "mbed-cloud-client/MbedCloudClient.h"
#include "MblCloudConnectIpcDBus.h"

namespace mbl {

class MblCloudConnectResourceBroker {

public:

    MblCloudConnectResourceBroker();
    virtual ~MblCloudConnectResourceBroker();

    // Initialize
    int Init();

    // Terminate
    int Terminate();

private:

    MblCloudConnectIpcDBus ipcDBus_;
 
};

} // namespace mbl

#endif // MblCloudConnectResourceBroker_h_
