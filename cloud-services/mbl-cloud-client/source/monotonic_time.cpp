/*
 * Copyright (c) 2017 Arm Limited and Contributors. All rights reserved.
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

#include "monotonic_time.h"

#include <cassert>

namespace mbl
{

time_t get_monotonic_time_s()
{
    struct timespec ts;
    const int cg_ret = clock_gettime(CLOCK_MONOTONIC, &ts);
    assert(cg_ret == 0);
    return ts.tv_sec;
}

} // namespace mbl
