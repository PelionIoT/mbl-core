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
#include "MblCloudConnectIpcDBus.h"
#include "mbed-trace/mbed_trace.h"

#include <cassert>
#include <pthread.h>

#define TRACE_GROUP "ccrb"

namespace mbl {

MblCloudConnectResourceBroker::MblCloudConnectResourceBroker() 
    : ipc_ (nullptr)
{
    tr_info("MblCloudConnectResourceBroker::MblCloudConnectResourceBroker");
}

MblCloudConnectResourceBroker::~MblCloudConnectResourceBroker()
{
    tr_info("MblCloudConnectResourceBroker::~MblCloudConnectResourceBroker");
}

MblError MblCloudConnectResourceBroker::init()
{
    // verify that ipc_ member was not created yet
    assert(nullptr == ipc_);
    tr_info("MblCloudConnectResourceBroker::init");

    // create ipc instance
    ipc_ = std::make_unique<MblCloudConnectIpcDBus>();

    MblError status = ipc_->init();
    if(Error::None != status) {
        tr_error("ipc::init failed with error %s", MblError_to_str(status));
    }

    return status;
}

MblError MblCloudConnectResourceBroker::run()
{
    assert(ipc_);
    tr_info("MblCloudConnectResourceBroker::run");

    MblError status = ipc_->run();
    if(Error::None != status) {
        tr_error("ipc::run failed with error %s", MblError_to_str(status));
    }

    return status;
}

void* MblCloudConnectResourceBroker::thread_function(void* ccrb_instance_ptr)
{
    assert(ccrb_instance_ptr);
    tr_info("MblCloudConnectResourceBroker::thread_function");

    MblCloudConnectResourceBroker * const ccrb_ptr =
        static_cast<MblCloudConnectResourceBroker*>(ccrb_instance_ptr);

    MblError status = ccrb_ptr->init();
    if(Error::None != status) {
        tr_error("ccrb::init failed with error %s", MblError_to_str(status));
        pthread_exit(nullptr);
    }

    status = ccrb_ptr->run();
    if(Error::None != status) {
        tr_error("ccrb::run failed with error %s", MblError_to_str(status));
        pthread_exit(nullptr);
    }

    tr_info("MblCloudConnectResourceBroker::thread_function finished");

    pthread_exit(nullptr);
    // pthread_exit does "return"
}

MblError MblCloudConnectResourceBroker::thread_join(void **args)
// param[in] args can be NULL
{
    assert(ipc_);
    tr_info("MblCloudConnectResourceBroker::thread_join");
    return ipc_->thread_join(args);
}

MblError MblCloudConnectResourceBroker::thread_finish()
{
    assert(ipc_);
    tr_info("MblCloudConnectResourceBroker::thread_finish");
    return ipc_->thread_finish();
}

} // namespace mbl

