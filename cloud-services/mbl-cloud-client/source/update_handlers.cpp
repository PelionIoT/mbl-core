/*
 * Copyright (c) 2017 ARM Ltd.
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

#include "update_handlers.h"

//#include "MblCloudClient.h"
#include "mbed-cloud-client/MbedCloudClient.h"

#include "mbed-trace/mbed_trace.h"

#include <inttypes.h>

#define TRACE_GROUP "mbl"

namespace mbl
{
namespace update_handlers
{

    static bool handle_download_request()
    {
        tr_info("Firmware download requested");
        tr_info("Authorization granted");
        return true;
    }

    static bool handle_install_request()
    {
        tr_info("Firmware install requested");
        tr_info("Authorization granted");
        return true;
    }

    void handle_download_progress(const uint32_t progress, const uint32_t total)
    {
        const unsigned percent = static_cast<unsigned>(progress * 100ULL / total);

        tr_info("Downloading: %u %%", percent);

        if (progress == total) {
            tr_info("Download completed");
        }
    }

    bool handle_authorize(const int32_t request)
    {
        switch (request)
        {
        case MbedCloudClient::UpdateRequestDownload: return handle_download_request();
        case MbedCloudClient::UpdateRequestInstall: return handle_install_request();
        }

        tr_warn("Uknown update authorization request (%" PRId32 ")", request);
        return false;
    }

} // namespace update_handlers
} // namespace mbl
