/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
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

#include "MblCloudConnectIpcInterface.h"

#include  <memory>

namespace mbl {

class MblCloudConnectResourceBroker {

public:

    MblCloudConnectResourceBroker();
    ~MblCloudConnectResourceBroker();

    // Initialize
    MblError Init();

private:

    std::unique_ptr<MblCloudConnectIpcInterface> ipc_;
 
};

} // namespace mbl

#endif // MblCloudConnectResourceBroker_h_
