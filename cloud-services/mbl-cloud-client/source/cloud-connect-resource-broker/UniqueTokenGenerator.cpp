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

#include "UniqueTokenGenerator.h"
#include "mbed-trace/mbed_trace.h"
#include <systemd/sd-id128.h>

#define TRACE_GROUP "ccrb-UniqueToken"

namespace mbl
{

MblError UniqueTokenGenerator::generate_unique_token(std::string& unique_token)
{
    sd_id128_t id128;
    int retval = sd_id128_randomize(&id128);
    if (retval != 0) {
        tr_err("sd_id128_randomize faile dwith error: %d", retval);
        return Error::CCRBGenerateUniqueIdFailed;
    }

    char buffer[33] = {0};
    unique_token = sd_id128_to_string(id128, buffer);
    return Error::None;
}

} // namespace mbl
