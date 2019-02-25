/*
 * Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
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

#include "pal.h"
#include "pal_plat_rtos.h"
#include <stdio.h>

#define TRACE_GROUP "mbl"

void pal_plat_osApplicationReboot(void)
{
    // Prevent reboot in case of application update (indicated by the presence
    // of the `do_not_reboot` file at `/tmp`).
    // It is a temporary solution until component update is supported and the
    // type of update package is provided in the manifest.
    FILE* fp = fopen("/tmp/do_not_reboot", "r");
    if (fp != NULL) {
        fclose(fp);
        PAL_LOG_INFO("Not rebooting the system (application update)\r\n");
        return;
    }

    PAL_LOG_INFO("Rebooting the system\r\n");
    pal_plat_osReboot();
}
