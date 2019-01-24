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

// Currently, this constructor is called from MblCloudClient thread.
MblCloudConnectResourceBroker::MblCloudConnectResourceBroker() 
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

MblCloudConnectResourceBroker::~MblCloudConnectResourceBroker()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

MblError MblCloudConnectResourceBroker::start()
{
    tr_info("%s", __PRETTY_FUNCTION__);

    pthread_t ccrb_thread_id = 0; // variable is not really used

    // create new thread which will run IPC event loop
    const int thread_create_err = pthread_create(
            &ccrb_thread_id,
            nullptr, // thread is created with default attributes
            MblCloudConnectResourceBroker::ccrb_main,
            this
        );
    if(0 != thread_create_err)
    {
        // thread creation failed, print errno value and exit
        const int thread_create_errno = errno;

        tr_err(
            "Thread creation failed (%s)!\n",
            strerror(thread_create_errno));

        return Error::CCRBStartFailed;
    }

    return Error::None;
}

MblError MblCloudConnectResourceBroker::stop()
{
    assert(ipc_);
    tr_info("%s", __PRETTY_FUNCTION__);

    MblError status = ipc_->stop();
    if(Error::None != status){
        tr_err("ipc::stop failed! (%s)", MblError_to_str(status));
        return status;
    }

    status = de_init();
    if(Error::None != status){
        tr_err("ccrb::de_init failed! (%s)", MblError_to_str(status));
    }

    return status;
}

MblError MblCloudConnectResourceBroker::init()
{
    // verify that ipc_ member was not created yet
    assert(nullptr == ipc_);
    tr_info("%s", __PRETTY_FUNCTION__);

    // create ipc instance
    ipc_ = std::make_unique<MblCloudConnectIpcDBus>();

    MblError status = ipc_->init();
    if(Error::None != status) {
        tr_error("ipc::init failed with error %s", MblError_to_str(status));
    }

    return status;
}

MblError MblCloudConnectResourceBroker::de_init()
{
    assert(ipc_);
    tr_info("%s", __PRETTY_FUNCTION__);

    MblError status = ipc_->de_init();
    if(Error::None != status) {
        tr_error("ipc::de_init failed with error %s", MblError_to_str(status));
    }

    return status;
}

MblError MblCloudConnectResourceBroker::run()
{
    assert(ipc_);
    tr_info("%s", __PRETTY_FUNCTION__);

    MblError status = ipc_->run();
    if(Error::None != status) {
        tr_error("ipc::run failed with error %s", MblError_to_str(status));
    }

    return status;
}

void* MblCloudConnectResourceBroker::ccrb_main(void* ccrb)
{
    assert(ccrb);
    tr_info("%s", __PRETTY_FUNCTION__);

    MblCloudConnectResourceBroker * const this_ccrb =
        static_cast<MblCloudConnectResourceBroker*>(ccrb);

    MblError status = this_ccrb->init();
    if(Error::None != status) {
        tr_error("ccrb::init failed with error %s. Exit CCRB thread.", MblError_to_str(status));
        pthread_exit(nullptr);
    }

    status = this_ccrb->run();
    if(Error::None != status) {
        tr_error("ccrb::run failed with error %s. Exit CCRB thread.", MblError_to_str(status));
        pthread_exit(nullptr);
    }

    tr_info("%s thread function finished", __PRETTY_FUNCTION__);
    pthread_exit(nullptr); // pthread_exit does "return"
}

} // namespace mbl

