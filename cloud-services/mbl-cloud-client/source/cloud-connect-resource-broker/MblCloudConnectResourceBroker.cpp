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

    // create new thread which will run IPC event loop
    const int thread_create_err = pthread_create(
            &ipc_thread_id_,
            nullptr, // thread is created with default attributes
            MblCloudConnectResourceBroker::ccrb_main,
            this
        );
    if(0 != thread_create_err) {
        // thread creation failed, print errno value and exit
        const int thread_create_errno = errno;

        tr_err(
            "Thread creation failed (%s)!\n",
            strerror(thread_create_errno));

        return Error::CCRBStartFailed;
    }

    // thread was created
    return Error::None;
}

MblError MblCloudConnectResourceBroker::stop()
{
    // FIXME: handle properly all errors in this function. 

    assert(ipc_);
    tr_info("%s", __PRETTY_FUNCTION__);
    
    // try sending stop signal to ipc
    const MblError ipc_stop_err = ipc_->stop();
    if(Error::None != ipc_stop_err) {
        tr_err("ipc::stop failed! (%s)", MblError_to_str(ipc_stop_err));

        // FIXME: Currently, if ipc was not succefully signalled, we return error.
        //        Required to add "relase resources best effort" functionality.
        return Error::CCRBStopFailed;
    }

    // about thread_status: thread_status is the variable that will contain ccrb_main status. 
    // ccrb_main returns MblError type. pthread_join will assign the value returned via 
    // pthread_exit to the value of pointer thread_status. For example, if ccrb_main 
    // will return N via pthread_exit(N), value of pointer thread_status will be equal N.
    void *thread_status = nullptr;     // do not dereference the pointer!

    // ipc was succefully signalled to stop, join with thread.
    const int thread_join_err = pthread_join(ipc_thread_id_, &thread_status);
    // FIXME: Currently, we use pthread_join without timeout. 
    //        Required to use pthread_join with timeout.
    if(0 != thread_join_err) {
        // thread joining failed, print errno value
        const int thread_join_errno = errno;
        tr_err(
            "Thread joining failed (%s)!\n",
            strerror(thread_join_errno));
        
        // FIXME: Currently, if pthread_join fails, we return error.
        //        Required to add "relase resources best effort" functionality.
        return Error::CCRBStopFailed;
    }

    // thread was succefully joined, handle thread_status. Thread_status will contain 
    // value that was returned via pthread_Exit(MblError) from ccrb main function.
    MblError ret_value = (MblError)(uintptr_t)(thread_status);
    if(Error::None != ret_value) {
        tr_err("ccrb_main() failed! (%s)", MblError_to_str(ret_value));
    }       

    tr_info("ccrb_main() exit status = (%s)", MblError_to_str(ret_value));

    const MblError de_init_err = de_init();
    if(Error::None != de_init_err) {
        tr_err("ccrb::de_init failed! (%s)", MblError_to_str(de_init_err));

        ret_value = (ret_value == Error::None)? 
                    de_init_err : 
                    ret_value;
    }

    return ret_value;
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
    
    // FIXME: Currently we call ipc_->de_init unconditionally. 
    //        ipc_->de_init can't be called if ccrb thread was not finished.
    //        Required to call ipc_->de_init only if ccrb thread was finished.  
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
        pthread_exit((void*)(uintptr_t)Error::CCRBStartFailed);
    }

    status = this_ccrb->run();
    if(Error::None != status) {
        tr_error("ccrb::run failed with error %s. Exit CCRB thread.", MblError_to_str(status));
        pthread_exit((void*)(uintptr_t)status);
    }

    tr_info("%s thread function finished", __PRETTY_FUNCTION__);
    pthread_exit((void*)(uintptr_t)Error::None); // pthread_exit does "return"
}

} // namespace mbl

