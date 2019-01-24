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

#ifndef MblCloudClient_h_
#define MblCloudClient_h_

#include "mbed-cloud-client/MbedCloudClient.h"

#include "MblError.h"
#include "MblMutex.h"
#include "cloud-connect-resource-broker/MblCloudConnectResourceBroker.h"

#include <stdint.h>

extern "C" void mbl_shutdown_handler(int signal);

namespace mbl {

class MblCloudClient {

public:
    static MblError run();

private:
    enum State
    {
        State_Unregistered,
        State_CalledRegister,
        State_Registered
    };

    struct InstanceScoper
    {
        InstanceScoper();
        ~InstanceScoper();
    };

    // Only InstanceScoper can create or destroy objects
    MblCloudClient();
    ~MblCloudClient();

    // No copying
    MblCloudClient(const MblCloudClient& other);
    MblCloudClient& operator=(const MblCloudClient& other);

    void register_handlers();
    void add_resources();
    MblError cloud_client_setup();

    static void handle_client_registered();
    static void handle_client_unregistered();
    static void handle_error(int error_code);
    static void handle_authorize(int32_t request);

    MbedCloudClient* cloud_client_;
    State state_;

    // Mbl Cloud Connect Resource Broker member
    MblCloudConnectResourceBroker cloud_connect_resource_broker_;

    static MblCloudClient* s_instance;
    static MblMutex s_mutex;
};

} // namespace mbl

#endif // MblCloudClient_h_
