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


#include "MblCloudConnectResourceBroker.h"
#include "MblCloudConnectIpcDBus.h"
#include "mbed-trace/mbed_trace.h"

#include <cassert>

#define TRACE_GROUP "CCRB"

namespace mbl {

MblCloudConnectResourceBroker::MblCloudConnectResourceBroker() 
    : ipc_ (std::make_unique<MblCloudConnectIpcDBus>())
{
    tr_debug("MblCloudConnectResourceBroker::MblCloudConnectResourceBroker");
}

MblCloudConnectResourceBroker::~MblCloudConnectResourceBroker()
{
    tr_debug("MblCloudConnectResourceBroker::~MblCloudConnectResourceBroker");
}

MblError MblCloudConnectResourceBroker::Init()
{
    tr_debug("MblCloudConnectResourceBroker::Init");

    assert(ipc_);
    MblError ret = ipc_->Init();
    if(Error::None != ret) {
        tr_error("Init ipc failed with error %s", MblError_to_str(ret));
    }
    return ret;
}

} // namespace mbl
