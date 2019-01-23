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

#include <systemd/sd-bus.h>

// FIXME: uncomment later
//#include "mbed-trace/mbed_trace.h"

#define TRACE_GROUP "ccrb-dbus"


int RegisterResourcesHandler(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{

}


// sd-bus vtable object, implements the com.mbed.Cloud.Connect1 interface
#define DBUS_CLOUD_SERVICE_NAME                 "com.mbed.Cloud"
#define DBUS_CLOUD_CONNECT_INTERFACE_NAME       "com.mbed.Cloud.Connect1"
#define DBUS_CLOUD_CONNECT_OBJECT_PATH          "/com/mbed/Cloud/Connect1"

static const sd_bus_vtable calculator_vtable[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("RegisterResources", "s", "", RegisterResourcesHandler, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_VTABLE_END
};

int bus_init(sd_bus *bus, sd_bus_slot *slot)
{
    int r = sd_bus_open_user(&bus);
    if (r < 0){
        return r;
    }
    if (NULL == bus){
        return -1;
    }

    /* Install the object */
    r = sd_bus_add_object_vtable(bus,
                                 &slot,
                                 DBUS_CLOUD_CONNECT_OBJECT_PATH,  
                                 DBUS_CLOUD_CONNECT_INTERFACE_NAME,
                                 calculator_vtable,
                                 NULL);
    if (r < 0) {
        return r;
    }

    r = sd_bus_get_unique_name(bus, &unique);
    if (r < 0) {
        log_error_errno(r, "Failed to get unique name: %m");
        goto fail;
    }

    /* Take a well-known service name so that clients can find us */
     r = sd_bus_request_name(bus, DBUS_CLOUD_SERVICE_NAME, 0);
    if (r < 0) {
        return r;
    }
        
    return 0;
}
