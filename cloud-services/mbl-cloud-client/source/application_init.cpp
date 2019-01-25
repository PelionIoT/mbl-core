// ----------------------------------------------------------------------------
// Copyright (c) 2016-2017 Arm Limited and Contributors. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------------------------------------------------------

#include "application_init.h"

#include "factory-configurator-client/factory_configurator_client.h"
#include "mbed-trace/mbed_trace.h"

#define TRACE_GROUP "mbl"

namespace mbl {

static bool application_init_fcc(void)
{
    fcc_status_e status = fcc_init();
    if(status != FCC_STATUS_SUCCESS) {
        tr_err("fcc_init failed with status %d! - exit", status);
        return 1;
    }

#ifdef MBED_CONF_APP_DEVELOPER_MODE
    tr_info("Start developer flow");
    status = fcc_developer_flow();
    if (status == FCC_STATUS_KCM_FILE_EXIST_ERROR) {
        tr_info("Developer credentials already exists");
    } else if (status != FCC_STATUS_SUCCESS) {
        tr_err("Failed to load developer credentials - exit");
        return 1;
    }
#endif
    status = fcc_verify_device_configured_4mbed_cloud();
    if (status != FCC_STATUS_SUCCESS) {
        tr_err("Device not configured for mbed Cloud - exit");
        return 1;
    }

    return 0;
}

bool application_init(void)
{
    tr_info("Start mbed Linux Cloud Client");

    if (application_init_fcc() != 0) {
        tr_err("Failed initializing FCC");
        return false;
    }

    return true;
}

} // namespace mbl
