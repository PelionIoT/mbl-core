/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ResourceBroker.h"
#include "DBusAdapter.h"
#include "MblCloudClient.h"
#include "mbed-cloud-client/MbedCloudClient.h"
#include "mbed-trace/mbed_trace.h"
#include <cassert>
#include <pthread.h>

#define TRACE_GROUP "ccrb"

namespace mbl
{

// Currently, this constructor is called from MblCloudClient thread.
ResourceBroker::ResourceBroker() : cloud_client_(nullptr), registration_in_progress_(false)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

ResourceBroker::~ResourceBroker()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

MblError ResourceBroker::start(MbedCloudClient* cloud_client)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    assert(cloud_client);
    cloud_client_ = cloud_client;

    // create new thread which will run IPC event loop
    const int thread_create_err =
        pthread_create(&ipc_thread_id_,
                       nullptr, // thread is created with default attributes
                       ResourceBroker::ccrb_main,
                       this);
    if (0 != thread_create_err) {
        // thread creation failed, print errno value and exit
        const int thread_create_errno = errno;
        tr_err("Thread creation failed (%s)", strerror(thread_create_errno));
        return Error::CCRBStartFailed;
    }

    // thread was created
    return Error::None;
}

MblError ResourceBroker::stop()
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    // FIXME: handle properly all errors in this function.

    assert(ipc_);

    // try sending stop signal to ipc
    const MblError ipc_stop_err = ipc_->stop();
    if (Error::None != ipc_stop_err) {
        tr_err("ipc::stop failed! (%s)", MblError_to_str(ipc_stop_err));

        // FIXME: Currently, if ipc was not successfully signalled, we return error.
        //        Required to add "release resources best effort" functionality.
        return Error::CCRBStopFailed;
    }

    // about thread_status: thread_status is the variable that will contain
    // ccrb_main status.
    // ccrb_main returns MblError type. pthread_join will assign the value
    // returned via pthread_exit to the value of pointer thread_status. For example, if
    // ccrb_main will return N via pthread_exit(N), value of pointer thread_status will be
    // equal N.
    void* thread_status = nullptr; // do not dereference the pointer!

    // ipc was succesfully signalled to stop, join with thread.
    const int thread_join_err = pthread_join(ipc_thread_id_, &thread_status);
    // FIXME: Currently, we use pthread_join without timeout.
    //        Required to use pthread_join with timeout.
    if (0 != thread_join_err) {
        // thread joining failed, print errno value
        const int thread_join_errno = errno;
        tr_err("Thread joining failed (%s)", strerror(thread_join_errno));

        // FIXME: Currently, if pthread_join fails, we return error.
        //        Required to add "release resources best effort" functionality.
        return Error::CCRBStopFailed;
    }

    // thread was succesfully joined, handle thread_status. Thread_status will
    // contain value that was returned via pthread_Exit(MblError) from ccrb main function.
    MblError ret_value = (MblError)(uintptr_t)(thread_status);
    if (Error::None != ret_value) {
        tr_err("ccrb_main() failed! (%s)", MblError_to_str(ret_value));
    }

    tr_info("ccrb_main() exit status = (%s)", MblError_to_str(ret_value));

    const MblError de_init_err = de_init();
    if (Error::None != de_init_err) {
        tr_err("ccrb::de_init failed! (%s)", MblError_to_str(de_init_err));

        ret_value = (ret_value == Error::None) ? de_init_err : ret_value;
    }

    return ret_value;
}

MblError ResourceBroker::init()
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    // verify that ipc_ member was not created yet
    assert(nullptr == ipc_);

    // create ipc instance and pass ccrb instance to constructor
    ipc_ = std::make_unique<DBusAdapter>(*this);

    MblError status = ipc_->init();
    if (Error::None != status) {
        tr_error("ipc::init failed with error %s", MblError_to_str(status));
    }

    // Set function pointers to point to mbed_client functions
    // In gtest our friend test class will override these pointers to get all the
    // calls
    register_update_func_ = std::bind(&MbedCloudClient::register_update, cloud_client_);
    add_objects_func_ = std::bind(
        static_cast<void (MbedCloudClient::*)(const M2MObjectList&)>(&MbedCloudClient::add_objects),
        cloud_client_,
        std::placeholders::_1);

    // Register Cloud client callback:
    regsiter_callback_handlers();

    return status;
}

MblError ResourceBroker::de_init()
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    assert(ipc_);

    // FIXME: Currently we call ipc_->de_init unconditionally.
    //        ipc_->de_init can't be called if ccrb thread was not finished.
    //        Required to call ipc_->de_init only if ccrb thread was finished.
    MblError status = ipc_->de_init();
    if (Error::None != status) {
        tr_error("ipc::de_init failed with error %s", MblError_to_str(status));
    }

    return status;
}

MblError ResourceBroker::run()
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    assert(ipc_);

    MblError status = ipc_->run();
    if (Error::None != status) {
        tr_error("ipc::run failed with error %s", MblError_to_str(status));
    }

    return status;
}

void* ResourceBroker::ccrb_main(void* ccrb)
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    assert(ccrb);

    ResourceBroker* const this_ccrb = static_cast<ResourceBroker*>(ccrb);

    MblError status = this_ccrb->init();
    if (Error::None != status) {
        tr_error("ccrb::init failed with error %s. Exit CCRB thread.", MblError_to_str(status));
        pthread_exit((void*) (uintptr_t) Error::CCRBStartFailed);
    }

    status = this_ccrb->run();
    if (Error::None != status) {
        tr_error("ccrb::run failed with error %s. Exit CCRB thread.", MblError_to_str(status));
        pthread_exit((void*) (uintptr_t) status);
    }

    tr_info("%s thread function finished", __PRETTY_FUNCTION__);
    pthread_exit((void*) (uintptr_t) Error::None); // pthread_exit does "return"
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Callback functions that are being called by Mbed client
////////////////////////////////////////////////////////////////////////////////////////////////////

void ResourceBroker::regsiter_callback_handlers()
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    // Resource broker will now get Mbed cloud client callbacks on the following
    // cases:
    // 1. Registration update
    // 2. Error occurred

    if (nullptr != cloud_client_) {
        // GTest unit test might set cloud_client_ member to nullptr
        cloud_client_->on_registration_updated(this,
                                               &ResourceBroker::handle_registration_updated_cb);
        cloud_client_->on_error(this, &ResourceBroker::handle_error_cb);
    }
}

void ResourceBroker::handle_registration_updated_cb()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    // Mark that registration is finished (using atomic flag)
    registration_in_progress_.store(false);
}

void ResourceBroker::handle_error_cb(const int cloud_client_code)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    const MblError mbl_code =
        CloudClientError_to_MblError(static_cast<MbedCloudClient::Error>(cloud_client_code));
    tr_err("%s: Error occurred: %d: %s", __PRETTY_FUNCTION__, mbl_code, MblError_to_str(mbl_code));

    // Mark that registration is finished (using atomic flag)
    registration_in_progress_.store(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Callback functions that are being called by ApplicationEndpoint Class
////////////////////////////////////////////////////////////////////////////////////////////////////

void ResourceBroker::handle_app_registration_updated(const uintptr_t ipc_conn_handle,
                                                     const std::string& access_token)
{
    tr_debug("%s: Application (access_token: %s) registered successfully.",
             __PRETTY_FUNCTION__,
             access_token.c_str());

    // Send the response to adapter:
    CloudConnectStatus reg_status = CloudConnectStatus::STATUS_SUCCESS;
    MblError status = ipc_->update_registration_status(ipc_conn_handle, reg_status);
    if (Error::None != status) {
        tr_error("%s: update_registration_status failed with error: %s",
                 __PRETTY_FUNCTION__,
                 MblError_to_str(status));
    }

    // Return registration updated callback to CCRB
    regsiter_callback_handlers();

    // Mark that registration is finished (using atomic flag)
    registration_in_progress_.store(false);
}

void ResourceBroker::handle_app_error_cb(const uintptr_t ipc_conn_handle,
                                         const std::string& access_token,
                                         const MblError error)
{
    tr_debug("%s: Application (access_token: %s) encountered an error: %s",
             __PRETTY_FUNCTION__,
             access_token.c_str(),
             MblError_to_str(error));

    auto itr = app_endpoints_map_.find(access_token);
    if (app_endpoints_map_.end() == itr) {
        // Could not found application endpoint
        tr_error("%s: Application (access_token: %s) does not exist.",
                 __PRETTY_FUNCTION__,
                 access_token.c_str());
        // Mark that registration is finished (using atomic flag), even if it was failed
        registration_in_progress_.store(false);
        return;
    }

    SPApplicationEndpoint app_endpoint = itr->second;

    // Send the response to adapter:
    if (app_endpoint->registered_) {
        // TODO: add call to update_deregistration_status when deregister is
        // implemented
        tr_error("%s: Application (access_token: %s) is already registered.",
                 __PRETTY_FUNCTION__,
                 access_token.c_str());
    }
    else
    {
        // App is not registered yet, which means the error is for register request
        MblError status =
            ipc_->update_registration_status(ipc_conn_handle, CloudConnectStatus::ERR_FAILED);
        if (Error::None != status) {
            tr_error("%s: Registration for Application (access_token: %s), failed "
                     "with error: %s",
                     __PRETTY_FUNCTION__,
                     access_token.c_str(),
                     MblError_to_str(status));
        }

        tr_debug(
            "%s: Erase Application (access_token: %s)", __PRETTY_FUNCTION__, access_token.c_str());
        app_endpoints_map_.erase(itr); // Erase endpoint as registation failed

        // Mark that registration is finished (using atomic flag) (even if it was failed)
        registration_in_progress_.store(false);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

MblError ResourceBroker::register_resources(const uintptr_t ipc_conn_handle,
                                            const std::string& app_resource_definition_json,
                                            CloudConnectStatus& out_status,
                                            std::string& out_access_token)
{
    tr_debug("%s", __PRETTY_FUNCTION__);

    if (registration_in_progress_.load()) {
        // We only allow one registration request at a time.
        tr_error("%s: Registration is already in progess.", __PRETTY_FUNCTION__);
        out_status = CloudConnectStatus::ERR_REGISTRATION_ALREADY_IN_PROGRESS;
        return Error::None;
    }

    // Above check makes sure there is no registration in progress.
    // Lets check if an app is already not registered...
    // TODO: remove this check when supporting multiple applications
    if (!app_endpoints_map_.empty()) {
        // Currently support only ONE application
        tr_error("%s: Only one registered application is allowed.", __PRETTY_FUNCTION__);
        out_status = CloudConnectStatus::ERR_ALREADY_REGISTERED;
        return Error::None;
    }

    // Create and init Application Endpoint:
    // parse app_resource_definition_json and create unique access token
    SPApplicationEndpoint app_endpoint =
        std::make_shared<ApplicationEndpoint>(ipc_conn_handle, *this);
    const MblError status = app_endpoint->init(app_resource_definition_json);
    if (Error::None != status) {
        tr_error("%s: app_endpoint->init failed with error: %s",
                 __PRETTY_FUNCTION__,
                 MblError_to_str(status));
        out_status = 
            (Error::CCRBInvalidJson == status) ? 
                CloudConnectStatus::ERR_INVALID_APPLICATION_RESOURCES_DEFINITION
                : CloudConnectStatus::ERR_FAILED;
        return Error::None;
    }

    out_access_token = app_endpoint->get_access_token();

    // Set atomic flag for registration in progress
    registration_in_progress_.store(true);

    // Register the next cloud client callbacks to app_endpoint
    if (nullptr != cloud_client_) {
		// GTest unit test might set cloud_client_ member to nullptr
        cloud_client_->on_registration_updated(
            &(*app_endpoint), &ApplicationEndpoint::handle_registration_updated_cb);
        cloud_client_->on_error(&(*app_endpoint), &ApplicationEndpoint::handle_error_cb);
    }

    app_endpoints_map_[out_access_token] = app_endpoint; // Add application endpoint to map

    // Call Mbed cloud client to start registration update
    add_objects_func_(app_endpoint->m2m_object_list_);
    register_update_func_();

    out_status = CloudConnectStatus::STATUS_SUCCESS;

    return Error::None;
}

MblError ResourceBroker::deregister_resources(const uintptr_t /*ipc_conn_handle*/,
                                              const std::string& /*access_token*/,
                                              CloudConnectStatus& /*out_status*/)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    // empty for now
    return Error::CCRBNotSupported;
}

MblError ResourceBroker::add_resource_instances(const uintptr_t /*unused*/,
                                                const std::string& /*unused*/,
                                                const std::string& /*unused*/,
                                                const std::vector<uint16_t>& /*unused*/,
                                                CloudConnectStatus& /*unused*/)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    // empty for now
    return Error::None;
}

MblError ResourceBroker::remove_resource_instances(const uintptr_t /*unused*/,
                                                   const std::string& /*unused*/,
                                                   const std::string& /*unused*/,
                                                   const std::vector<uint16_t>& /*unused*/,
                                                   CloudConnectStatus& /*unused*/)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    // empty for now
    return Error::None;
}

MblError ResourceBroker::set_resources_values(const std::string& /*unused*/,
                                              std::vector<ResourceSetOperation>& /*unused*/,
                                              CloudConnectStatus& /*unused*/)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    // empty for now
    return Error::None;
}

MblError ResourceBroker::get_resources_values(const std::string& /*unused*/,
                                              std::vector<ResourceGetOperation>& /*unused*/,
                                              CloudConnectStatus& /*unused*/)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    // empty for now
    return Error::None;
}

} // namespace mbl
