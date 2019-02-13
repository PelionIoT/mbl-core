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
#include "mbed-cloud-client/MbedCloudClient.h"
#include "mbed-trace/mbed_trace.h"

#define TRACE_GROUP "ccrb-app-end-point"

namespace mbl {

ApplicationEndpoint::ApplicationEndpoint(std::string access_token, ResourceBroker &ccrb)
: access_token_(std::move(access_token)), 
ccrb_(ccrb),
registered_(false)
{
    tr_debug("@@@@@@ %s: out_access_token: %s", __PRETTY_FUNCTION__, access_token_.c_str());
}

ApplicationEndpoint::~ApplicationEndpoint()
{
    tr_debug("@@@@@@ %s", __PRETTY_FUNCTION__);
}

void ApplicationEndpoint::regsiter_callback_handlers()
{
    tr_debug("@@@@@@ %s - Call on_registration_updated()", __PRETTY_FUNCTION__);
    ccrb_.cloud_client_->on_registration_updated(this, &ApplicationEndpoint::handle_app_registered_cb);
    ccrb_.cloud_client_->on_unregistered(this, &ApplicationEndpoint::handle_app_unregistered_cb);
    ccrb_.cloud_client_->on_error(this, &ApplicationEndpoint::handle_app_error_cb);
}

void ApplicationEndpoint::handle_app_registered_cb()
{
    tr_debug("@@@@@@ %s: calling handle_app_registered_cb(access_token = %s)", __PRETTY_FUNCTION__, access_token_.c_str());
    registered_ = true;
    ccrb_.handle_app_registered_cb(access_token_);
}

void ApplicationEndpoint::handle_app_unregistered_cb()
{
    tr_debug("@@@@@@ %s: calling handle_app_unregistered_cb(access_token = %s)", __PRETTY_FUNCTION__, access_token_.c_str());
    registered_ = false;
    ccrb_.handle_app_unregistered_cb(access_token_);
}

void ApplicationEndpoint::handle_app_error_cb(const int cloud_client_code)
{
    const MblError mbl_code = CloudClientError_to_MblError(static_cast<MbedCloudClient::Error>(cloud_client_code));
    tr_err("%s: Error occurred: %d: %s", 
        __PRETTY_FUNCTION__,
        mbl_code,
        MblError_to_str(mbl_code));
    tr_debug("@@@@@@ %s: calling handle_app_error_cb(access_token = %s)", __PRETTY_FUNCTION__, access_token_.c_str());
    ccrb_.handle_app_error_cb(access_token_, mbl_code);
}


std::string ApplicationEndpoint::get_access_token() const
{
    return access_token_;
}

bool ApplicationEndpoint::is_registered()
{
    return registered_;
}

} // namespace mbl
