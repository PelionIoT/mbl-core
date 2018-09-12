/*
 * Copyright (c) 2018 ARM Ltd.
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

#include <stdio.h>
#include "pal.h"
#include "pal_plat_rtos.h"

#define TRACE_GROUP "mbl"


void pal_plat_osApplicationReboot(void)
{
    // Hack application reboot for demo, this should ne changed later on when we have the information about the type of update in the manifest
    FILE *fp = fopen ("/tmp/do_not_reboot", "r");
    if (fp != NULL)
    {
        fclose (fp);
        PAL_LOG(INFO, "Not rebooting the system (application update)\r\n");
        return;
    }
    
    PAL_LOG(INFO, "Rebooting the system\r\n");
    pal_plat_osReboot();
}
