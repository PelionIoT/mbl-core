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


#include "MblCloudConnectResourceBroker.h"
#include "mbed-trace/mbed_trace.h"

#define TRACE_GROUP "CCRB"

namespace mbl {

MblCloudConnectResourceBroker::MblCloudConnectResourceBroker()
{
    tr_debug("MblCloudConnectResourceBroker::MblCloudConnectResourceBroker");
}

MblCloudConnectResourceBroker::~MblCloudConnectResourceBroker()
{
    tr_debug("MblCloudConnectResourceBroker::~MblCloudConnectResourceBroker");
}

int MblCloudConnectResourceBroker::Init()
{
    tr_debug("MblCloudConnectResourceBroker::Init");
    int ret = ipcDBus_.Init();
    if(0 != ret) {
        tr_error("Init ipcDBus failed with error %d", ret);
    }
    return ret;
}

int MblCloudConnectResourceBroker::Terminate()
{
    tr_debug("MblCloudConnectResourceBroker::Terminate");
    int ret = ipcDBus_.Terminate();
    if(0 != ret) {
        tr_error("Terminate ipcDBus failed with error %d", ret);
    }
    return ret;
}

} // namespace mbl
