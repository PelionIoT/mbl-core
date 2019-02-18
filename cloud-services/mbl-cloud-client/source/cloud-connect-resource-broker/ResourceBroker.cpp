/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cassert>
#include <pthread.h>

#include "ResourceBroker.h"
#include "DBusAdapter.h"
#include "CloudConnectCommon_Internal.h"

#define TRACE_GROUP "ccrb"

namespace mbl {

// Currently, this constructor is called from MblCloudClient thread.
ResourceBroker::ResourceBroker() 
{
    tr_debug("Enter");
}

ResourceBroker::~ResourceBroker()
{
    tr_debug("Enter");
}

MblError ResourceBroker::start()
{
    tr_debug("Enter");

    // create new thread which will run IPC event loop
    const int thread_create_err = pthread_create(
            &ipc_thread_id_,
            nullptr, // thread is created with default attributes
            ResourceBroker::ccrb_main,
            this
        );
    if(0 != thread_create_err) {
        // thread creation failed, print errno value and exit
        tr_err("Thread creation failed (%s)", strerror(errno));
        return Error::CCRBStartFailed;
    }

    // thread was created
    return Error::None;
}

MblError ResourceBroker::stop()
{
    tr_debug("Enter");
    
    // FIXME: handle properly all errors in this function. 

    assert(ipc_adapter_);
    
    // try sending stop signal to ipc (pass no error)
    const MblError ipc_stop_err = ipc_adapter_->stop(MblError::None);
    if(Error::None != ipc_stop_err) {
        tr_err("ipc::stop failed! (%s)", MblError_to_str(ipc_stop_err));

        // FIXME: Currently, if ipc was not successfully signalled, we return error.
        //        Required to add "release resources best effort" functionality.
        return Error::CCRBStopFailed;
    }

    // about thread_status: thread_status is the variable that will contain ccrb_main status. 
    // ccrb_main returns MblError type. pthread_join will assign the value returned via 
    // pthread_exit to the value of pointer thread_status. For example, if ccrb_main 
    // will return N via pthread_exit(N), value of pointer thread_status will be equal N.
    void *thread_status = nullptr;     // do not dereference the pointer!

    // ipc was succesfully signalled to stop, join with thread.
    const int thread_join_err = pthread_join(ipc_thread_id_, &thread_status);
    // FIXME: Currently, we use pthread_join without timeout. 
    //        Required to use pthread_join with timeout.
    if(0 != thread_join_err) {
        // thread joining failed, print errno value
        tr_err("Thread joining failed (%s)", strerror(errno));
        
        // FIXME: Currently, if pthread_join fails, we return error.
        //        Required to add "release resources best effort" functionality.
        return Error::CCRBStopFailed;
    }

    // thread was succesfully joined, handle thread_status. Thread_status will contain 
    // value that was returned via pthread_Exit(MblError) from ccrb main function.
    MblError ret_value = (MblError)(uintptr_t)(thread_status);
    if(Error::None != ret_value) {
        tr_err("ccrb_main() failed! (%s)", MblError_to_str(ret_value));
    }       

    tr_info("ccrb_main() exit status = (%s)", MblError_to_str(ret_value));

    const MblError de_init_err = deinit();
    if(Error::None != de_init_err) {
        tr_err("ccrb::de_init failed! (%s)", MblError_to_str(de_init_err));

        ret_value = (ret_value == Error::None)? 
                    de_init_err : 
                    ret_value;
    }

    return ret_value;
}

MblError ResourceBroker::init()
{
    tr_debug("Enter");
    
    // verify that ipc_adapter_ member was not created yet
    assert(nullptr == ipc_adapter_);

    // create ipc instance and pass ccrb instance to constructor
    ipc_adapter_ = std::make_unique<DBusAdapter>(*this);

    MblError status = ipc_adapter_->init();
    if(Error::None != status) {
        tr_error("ipc::init failed with error %s", MblError_to_str(status));
    }

    return status;
}

MblError ResourceBroker::deinit()
{
    tr_debug("Enter");
    
    assert(ipc_adapter_);
   
    // FIXME: Currently we call ipc_adapter_->de_init unconditionally. 
    //        ipc_adapter_->de_init can't be called if ccrb thread was not finished.
    //        Required to call ipc_adapter_->de_init only if ccrb thread was finished.  
    MblError status = ipc_adapter_->deinit();
    if(Error::None != status) {
        tr_error("ipc::de_init failed with error %s", MblError_to_str(status));
    }

    return status;
}

MblError ResourceBroker::run()
{
    tr_debug("Enter");
    MblError stop_status;
    
    assert(ipc_adapter_);

    MblError status = ipc_adapter_->run(stop_status);
    if(Error::None != status) {
        tr_error("ipc::run failed with error %s", MblError_to_str(status));
    }
    else {
        tr_error("ipc::run stopped with status %s", MblError_to_str(stop_status));
    }

    return status;
}

void* ResourceBroker::ccrb_main(void* ccrb)
{
    tr_debug("Enter");
    
    assert(ccrb);

    ResourceBroker * const this_ccrb =
        static_cast<ResourceBroker*>(ccrb);

    MblError status = this_ccrb->init();
    if(Error::None != status) {
        tr_error("ccrb::init failed with error %s. Exit CCRB thread.", MblError_to_str(status));
        pthread_exit((void*)(uintptr_t)Error::CCRBStartFailed);
    }

    status = this_ccrb->run();
    if(Error::None != status) {
        tr_error("ccrb::run failed with error %s. Exit CCRB thread.", MblError_to_str(status));
        //continue to deinit and return status
    }

    MblError status1 = this_ccrb->deinit();
    if(Error::None != status1) {
        tr_error("ccrb::deinit failed with error %s. Exit CCRB thread.", MblError_to_str(status1));
        pthread_exit((void*)(uintptr_t)Error::CCRBStartFailed);
    }
    
    tr_info("%s thread function finished", __PRETTY_FUNCTION__);
    pthread_exit((void*)(uintptr_t)status); // pthread_exit does "return"
}

const char* CloudConnectStatus_to_readable_string(const CloudConnectStatus status)
{
    switch (status)
    {
        case STATUS_SUCCESS: 
            return "STATUS SUCCESS";

        case ERR_FAILED: 
            return "ERROR FAILED";

        case ERR_INTERNAL_ERROR:
            return "INTERNAL ERROR IN CLOUD CONNECT INFRASTRUCTURE";

        default:
            return "Unknown Cloud Connect Status or Error";
    }
}

MblError ResourceBroker::register_resources(
        const uintptr_t /*unused*/, 
        const std::string & json/*unused*/,
        CloudConnectStatus & status /*unused*/,
        std::string &token /*unused*/)
{
    tr_debug("Enter");

    status = (CloudConnectStatus)atoi(json.c_str());
    token = std::string(json) + std::string(json) + std::string(json) + std::string(json);

    // empty for now
    return Error::None;
}


MblError ResourceBroker::deregister_resources(
        const uintptr_t /*unused*/, 
        const std::string & token/*unused*/,
        CloudConnectStatus & status /*unused*/)
{
    tr_debug("Enter");    
    status = (CloudConnectStatus)atoi(token.c_str());
    // empty for now
    return Error::None;
}

MblError ResourceBroker::add_resource_instances(
        const uintptr_t /*unused*/, 
        const std::string & /*unused*/, 
        const std::string & /*unused*/, 
        const std::vector<uint16_t> & /*unused*/,
        CloudConnectStatus & /*unused*/)
{
    tr_debug("Enter");
    // empty for now
    return Error::None;
}

MblError ResourceBroker::remove_resource_instances(
    const uintptr_t /*unused*/, 
    const std::string & /*unused*/, 
    const std::string & /*unused*/, 
    const std::vector<uint16_t> & /*unused*/,
        CloudConnectStatus & /*unused*/)
{
    tr_debug("Enter");    
    // empty for now
    return Error::None;
}

MblError ResourceBroker::set_resources_values(
        const std::string & /*unused*/, 
        std::vector<ResourceSetOperation> & /*unused*/,
        CloudConnectStatus & /*unused*/)
{
    tr_debug("Enter");    
    // empty for now
    return Error::None;
}

MblError ResourceBroker::get_resources_values(
        const std::string & /*unused*/, 
        std::vector<ResourceGetOperation> & /*unused*/,
        CloudConnectStatus & /*unused*/)
{
    tr_debug("Enter");
    // empty for now
    return Error::None;
}

} // namespace mbl
