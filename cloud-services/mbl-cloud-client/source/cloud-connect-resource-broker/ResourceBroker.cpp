/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ResourceBroker.h"
#include "CloudConnectTrace.h"
#include "DBusAdapter.h"

#include <cassert>
#include <pthread.h>

#define TRACE_GROUP "ccrb"

namespace mbl
{

// Currently, this constructor is called from MblCloudClient thread.
ResourceBroker::ResourceBroker()
{
    TR_DEBUG("Enter");
}

ResourceBroker::~ResourceBroker()
{
    TR_DEBUG("Enter");
}

MblError ResourceBroker::start()
{
    TR_DEBUG("Enter");

    // create new thread which will run IPC event loop
    const int thread_create_err =
        pthread_create(&ipc_thread_id_,
                       nullptr, // thread is created with default attributes
                       ResourceBroker::ccrb_main,
                       this);
    if (0 != thread_create_err) {
        // thread creation failed, print errno value and exit
        TR_ERR("Thread creation failed (%s)", strerror(errno));
        return Error::CCRBStartFailed;
    }

    // thread was created
    return Error::None;
}

MblError ResourceBroker::stop()
{
    TR_DEBUG("Enter");

    // FIXME: handle properly all errors in this function.

    assert(ipc_adapter_);

    // try sending stop signal to ipc (pass no error)
    const MblError ipc_stop_err = ipc_adapter_->stop(MblError::None);
    if (Error::None != ipc_stop_err) {
        TR_ERR("ipc::stop failed! (%s)", MblError_to_str(ipc_stop_err));

        // FIXME: Currently, if ipc was not successfully signalled, we return error.
        //        Required to add "release resources best effort" functionality.
        return Error::CCRBStopFailed;
    }

    // about thread_status: thread_status is the variable that will contain ccrb_main status.
    // ccrb_main returns MblError type. pthread_join will assign the value returned via
    // pthread_exit to the value of pointer thread_status. For example, if ccrb_main
    // will return N via pthread_exit(N), value of pointer thread_status will be equal N.
    void* thread_status = nullptr; // do not dereference the pointer!

    // ipc was succesfully signalled to stop, join with thread.
    const int thread_join_err = pthread_join(ipc_thread_id_, &thread_status);
    // FIXME: Currently, we use pthread_join without timeout.
    //        Required to use pthread_join with timeout.
    if (0 != thread_join_err) {
        // thread joining failed, print errno value
        TR_ERR("Thread joining failed (%s)", strerror(errno));

        // FIXME: Currently, if pthread_join fails, we return error.
        //        Required to add "release resources best effort" functionality.
        return Error::CCRBStopFailed;
    }

    // thread was succesfully joined, handle thread_status. Thread_status will contain
    // value that was returned via pthread_Exit(MblError) from ccrb main function.
    MblError* ret_value = static_cast<MblError*>(thread_status);
    if (Error::None != (*ret_value)) {
        // the return with an error ret_Value indicate of the reason for exit, but not the
        // failure of the current function.
        TR_ERR("ccrb_main() exit with error (*ret_value)= %s", MblError_to_str((*ret_value)));
    }

    const MblError de_init_err = deinit();
    if (Error::None != de_init_err) {
        TR_ERR("ccrb::de_init failed! (%s)", MblError_to_str(de_init_err));
    }

    return de_init_err;
}

MblError ResourceBroker::init()
{
    TR_DEBUG("Enter");

    // verify that ipc_adapter_ member was not created yet
    assert(nullptr == ipc_adapter_);

    // create ipc instance and pass ccrb instance to constructor
    ipc_adapter_ = std::make_unique<DBusAdapter>(*this);

    MblError status = ipc_adapter_->init();
    if (Error::None != status) {
        TR_ERR("ipc::init failed with error %s", MblError_to_str(status));
    }

    return status;
}

MblError ResourceBroker::deinit()
{
    TR_DEBUG("Enter");

    assert(ipc_adapter_);

    // FIXME: Currently we call ipc_adapter_->de_init unconditionally.
    //        ipc_adapter_->de_init can't be called if ccrb thread was not finished.
    //        Required to call ipc_adapter_->de_init only if ccrb thread was finished.
    MblError status = ipc_adapter_->deinit();
    if (Error::None != status) {
        TR_ERR("ipc::de_init failed with error %s", MblError_to_str(status));
    }

    return status;
}

MblError ResourceBroker::run()
{
    TR_DEBUG("Enter");
    MblError stop_status;

    assert(ipc_adapter_);

    MblError status = ipc_adapter_->run(stop_status);
    if (Error::None != status) {
        TR_ERR("ipc::run failed with error %s", MblError_to_str(status));
    }
    else
    {
        TR_ERR("ipc::run stopped with status %s", MblError_to_str(stop_status));
    }

    return status;
}

void* ResourceBroker::ccrb_main(void* ccrb)
{
    TR_DEBUG("Enter");

    assert(ccrb);

    ResourceBroker* const this_ccrb = static_cast<ResourceBroker*>(ccrb);

    MblError status = this_ccrb->init();
    if (Error::None != status) {
        TR_ERR("ccrb::init failed with error %s. Exit CCRB thread.", MblError_to_str(status));
        pthread_exit((void*) (uintptr_t) status);
    }

    status = this_ccrb->run();
    if (Error::None != status) {
        TR_ERR("ccrb::run failed with error %s. Exit CCRB thread.", MblError_to_str(status));
        // continue to deinit and return status
    }

    MblError status1 = this_ccrb->deinit();
    if (Error::None != status1) {
        TR_ERR("ccrb::deinit failed with error %s. Exit CCRB thread.", MblError_to_str(status1));
        pthread_exit((void*) &status1);
    }

    TR_INFO("thread function finished");
    pthread_exit((void*) (uintptr_t) status); // pthread_exit does "return"
}

MblError ResourceBroker::register_resources(const uintptr_t /*unused*/,
                                            const std::string& /*unused*/,
                                            CloudConnectStatus& /*unused*/,
                                            std::string& /*unused*/)
{
    TR_DEBUG("Enter");
    // empty for now
    return Error::None;
}

MblError ResourceBroker::deregister_resources(const uintptr_t /*unused*/,
                                              const std::string& /*unused*/,
                                              CloudConnectStatus& /*unused*/)
{
    TR_DEBUG("Enter");
    // empty for now
    return Error::None;
}

MblError ResourceBroker::add_resource_instances(const uintptr_t /*unused*/,
                                                const std::string& /*unused*/,
                                                const std::string& /*unused*/,
                                                const std::vector<uint16_t>& /*unused*/,
                                                CloudConnectStatus& /*unused*/)
{
    TR_DEBUG("Enter");
    // empty for now
    return Error::None;
}

MblError ResourceBroker::remove_resource_instances(const uintptr_t /*unused*/,
                                                   const std::string& /*unused*/,
                                                   const std::string& /*unused*/,
                                                   const std::vector<uint16_t>& /*unused*/,
                                                   CloudConnectStatus& /*unused*/)
{
    TR_DEBUG("Enter");
    // empty for now
    return Error::None;
}

MblError ResourceBroker::set_resources_values(const std::string& /*unused*/,
                                              std::vector<ResourceSetOperation>& /*unused*/,
                                              CloudConnectStatus& /*unused*/)
{
    TR_DEBUG("Enter");
    // empty for now
    return Error::None;
}

MblError ResourceBroker::get_resources_values(const std::string& /*unused*/,
                                              std::vector<ResourceGetOperation>& /*unused*/,
                                              CloudConnectStatus& /*unused*/)
{
    TR_DEBUG("Enter");
    // empty for now
    return Error::None;
}

} // namespace mbl
