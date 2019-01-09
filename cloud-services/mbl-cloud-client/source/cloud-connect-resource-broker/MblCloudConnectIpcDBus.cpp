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

#define TRACE_GROUP "CCRB-IPCDBUS"

namespace mbl {

MblCloudConnectIpcDBus::MblCloudConnectIpcDBus()
{
    tr_debug("MblCloudConnectIpcDBus::MblCloudConnectIpcDBus");

    if( __cplusplus == 1 ) {
        tr_warning("@@@@@@@@@@ pre-C++98");
    }
    else if( __cplusplus == 199711L ) {
        tr_warning("@@@@@@@@@@ C++98");
    }
    else if( __cplusplus == 201103L ) {
        tr_warning("@@@@@@@@@@ C++11");
    }
    else if( __cplusplus == 201402L ) {
        tr_warning("@@@@@@@@@@ C++14");
    }
    else if( __cplusplus == 201703L ) {
        tr_warning("@@@@@@@@@@ C++17");
    }
    else {
        tr_warning("@@@@@@@@@@ N/A");
    }
}

MblCloudConnectIpcDBus::~MblCloudConnectIpcDBus()
{
    tr_debug("MblCloudConnectIpcDBus::~MblCloudConnectIpcDBus");
}

int MblCloudConnectIpcDBus::Init()
{
    tr_debug("MblCloudConnectIpcDBus::Init");
    return 0;
}

int MblCloudConnectIpcDBus::Terminate()
{
    tr_debug("MblCloudConnectIpcDBus::Terminate");
    return 0;
}

} // namespace mbl
