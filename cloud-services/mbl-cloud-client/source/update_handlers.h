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

#ifndef update_handlers_h_
#define update_handlers_h_

#include <stdint.h>

namespace mbl {
namespace update_handlers {

/**
 * Handler for when the server requests a firmware download or firmware
 * install.
 *
 * @return true if the request is authorized; false otherwise.
 */
bool handle_authorize(int32_t request);

/**
 * Handler for when download progress is available
 */
void handle_download_progress(uint32_t progress, uint32_t total);

} // namespace update_handlers
} // namespace mbl

#endif // update_handlers_h_
