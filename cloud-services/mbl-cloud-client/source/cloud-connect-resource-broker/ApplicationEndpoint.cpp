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

#include "ApplicationEndpoint.h"
#include "CloudConnectTrace.h"
#include "ResourceDefinitionParser.h"
#include "mbed-cloud-client/MbedCloudClient.h"
#include <systemd/sd-id128.h>

#define TRACE_GROUP "ccrb-app-end-point"

namespace mbl
{

ApplicationEndpoint::ApplicationEndpoint(const uintptr_t ipc_conn_handle)
    : ipc_conn_handle_(ipc_conn_handle),
      registered_(false),
      handle_app_register_update_finished_cb_(nullptr),
      handle_app_error_cb_(nullptr)
{
    TR_DEBUG("Enter");
}

ApplicationEndpoint::~ApplicationEndpoint()
{
    TR_DEBUG("(access_token: %s)", access_token_.c_str());
}

void ApplicationEndpoint::register_callback_functions(
    app_register_update_finished_func register_update_finished_func, app_error_func error_func)
{
    handle_app_register_update_finished_cb_ = register_update_finished_func;
    handle_app_error_cb_ = error_func;
}

MblError ApplicationEndpoint::generate_access_token()
{
    TR_DEBUG("Enter");

    sd_id128_t id128 = SD_ID128_NULL;
    int retval = sd_id128_randomize(&id128);
    if (retval != 0) {
        TR_ERR("sd_id128_randomize failed with error: %d", retval);
        return Error::CCRBGenerateUniqueIdFailed;
    }

    char buffer[33] = {0};
    access_token_ = sd_id128_to_string(id128, buffer);
    return Error::None;
}

MblError ApplicationEndpoint::init(const std::string& application_resource_definition)
{
    TR_DEBUG("Enter");

    RBM2MObjectList rbm2m_object_list; // TODO: should be remove in future task
    // Parse JSON
    ResourceDefinitionParser resource_parser;
    const MblError build_object_list_stauts = resource_parser.build_object_list(
        application_resource_definition, m2m_object_list_, rbm2m_object_list);

    if (Error::None != build_object_list_stauts) {
        TR_ERR("build_object_list failed with error: %s",
               MblError_to_str(build_object_list_stauts));
        return build_object_list_stauts;
    }

    const MblError generate_access_token_status = generate_access_token();
    if (Error::None != generate_access_token_status) {
        TR_ERR("generate_access_token failed with error: %s",
               MblError_to_str(generate_access_token_status));
        return generate_access_token_status;
    }
    TR_DEBUG("(access_token: %s) succeeded.", access_token_.c_str());
    return Error::None;
}

void ApplicationEndpoint::handle_registration_updated_cb()
{
    TR_DEBUG("(access_token: %s) - Notify CCRB that registration was successfull.",
             access_token_.c_str());
    registered_ = true;
    if (nullptr != handle_app_register_update_finished_cb_) {
        handle_app_register_update_finished_cb_(ipc_conn_handle_, access_token_);
    }
    else
    {
        TR_ERR("handle_app_register_update_finished_cb_ was not set.");
    }
}

void ApplicationEndpoint::handle_error_cb(const int cloud_client_code)
{
    const MblError mbl_code =
        CloudClientError_to_MblError(static_cast<MbedCloudClient::Error>(cloud_client_code));
    TR_ERR("(access_token: %s) - Error occurred: %d: %s",
           access_token_.c_str(),
           mbl_code,
           MblError_to_str(mbl_code));

    TR_DEBUG("(access_token: %s) - Notify CCRB that error occured.", access_token_.c_str());

    if (nullptr != handle_app_error_cb_) {
        handle_app_error_cb_(ipc_conn_handle_, access_token_, mbl_code);
    }
    else
    {
        TR_ERR("handle_app_error_cb_ was not set.");
    }
}

} // namespace mbl
