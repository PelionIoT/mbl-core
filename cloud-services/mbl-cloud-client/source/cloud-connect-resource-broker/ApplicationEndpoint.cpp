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

#include "ResourceBroker.h"
#include "ApplicationEndpoint.h"
#include "ResourceDefinitionParser.h"
#include "mbed-cloud-client/MbedCloudClient.h"
#include "mbed-trace/mbed_trace.h"

#define TRACE_GROUP "ccrb-app-end-point"

namespace mbl {

ApplicationEndpoint::ApplicationEndpoint(
    const uintptr_t ipc_conn_handle,
    std::string access_token,
    ResourceBroker &ccrb)
: ipc_conn_handle_(ipc_conn_handle),
access_token_(std::move(access_token)), 
ccrb_(ccrb),
registered_(false)
{
    tr_debug("%s: out_access_token: %s", __PRETTY_FUNCTION__, access_token_.c_str());
}

ApplicationEndpoint::~ApplicationEndpoint()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

MblError ApplicationEndpoint::init(const std::string json_string)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    
    // Parse JSON
    ResourceDefinitionParser resource_parser;
    const MblError status = resource_parser.build_object_list(
        json_string, 
        m2m_object_list_,
        rbm2m_object_list_);

    if(Error::None != status) {
        tr_error("%s: build_object_list failed with error: %s",
            __PRETTY_FUNCTION__,
            MblError_to_str(status));
    }
    return status;
}

void ApplicationEndpoint::update_ipc_conn_handle(const uintptr_t ipc_conn_handle)
{
    ipc_conn_handle_ = ipc_conn_handle;
}

void ApplicationEndpoint::handle_register_cb()
{
    tr_debug("%s: Notify CCRB that registration was successfull (access_token = %s)",
        __PRETTY_FUNCTION__,
        access_token_.c_str());
    registered_ = true;
    ccrb_.handle_app_register_cb(ipc_conn_handle_, access_token_);
}

void ApplicationEndpoint::handle_error_cb(const int cloud_client_code)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    const MblError mbl_code = CloudClientError_to_MblError(static_cast<MbedCloudClient::Error>(cloud_client_code));
    tr_err("%s: Error occurred: %d: %s", 
        __PRETTY_FUNCTION__,
        mbl_code,
        MblError_to_str(mbl_code));

    tr_debug("%s: Notify CCRB that error occured (access_token = %s)",
        __PRETTY_FUNCTION__,
        access_token_.c_str());
    ccrb_.handle_app_error_cb(ipc_conn_handle_, access_token_, mbl_code);
}

uintptr_t ApplicationEndpoint::get_ipc_conn_handle() const
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    return ipc_conn_handle_;
}

std::string ApplicationEndpoint::get_access_token() const
{
    tr_debug("%s: (access_token = %s)", __PRETTY_FUNCTION__, access_token_.c_str());
    return access_token_;
}

bool ApplicationEndpoint::is_registered()
{
    tr_debug("%s: (access_token = %s) registered = %d",
        __PRETTY_FUNCTION__,
        access_token_.c_str(),
        registered_);
    return registered_;
}

M2MObjectList& ApplicationEndpoint::get_m2m_object_list()
{
    return m2m_object_list_;
}

} // namespace mbl
