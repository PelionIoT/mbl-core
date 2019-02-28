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

#include "RegistrationRecord.h"
#include "CloudConnectTrace.h"
#include "ResourceDefinitionParser.h"
#include "mbed-cloud-client/MbedCloudClient.h"

#define TRACE_GROUP "ccrb-registration-record"

namespace mbl
{

RegistrationRecord::RegistrationRecord(const uintptr_t ipc_request_handle)
    : ipc_request_handle_(ipc_request_handle), registered_(false)
{
    TR_DEBUG("Enter");
}

RegistrationRecord::~RegistrationRecord()
{
    TR_DEBUG("Enter");
}

MblError RegistrationRecord::init(const std::string& application_resource_definition)
{
    TR_DEBUG("Enter");

    // Parse JSON
    const MblError build_object_list_stauts = ResourceDefinitionParser::build_object_list(
        application_resource_definition, m2m_object_list_);

    if (Error::None != build_object_list_stauts) {
        TR_ERR("build_object_list failed with error: %s",
               MblError_to_str(build_object_list_stauts));
        return build_object_list_stauts;
    }
    return Error::None;
}

} // namespace mbl
