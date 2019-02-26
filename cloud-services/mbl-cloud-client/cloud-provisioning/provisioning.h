/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
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

#ifndef PROVISIONING_H
#define PROVISIONING_H

#include <vector>
#include <string>
#include <cstdint>
#include "factory_configurator_client.h"
#include "key_config_manager.h"


namespace mbl{
    namespace provisioning{
        enum class ProvisionedStatusCode
        {
            success = 0,
            failure = 1
        };

        // KCM storage item.
        struct KCMItem
        {
            const std::string           name;
            const kcm_item_type_e       type;
            const std::vector<uint8_t>  data_blob;
        };

        // Load developer credentials certificate.
        std::pair<std::vector<KCMItem>, ProvisionedStatusCode> load_developer_cloud_credentials();
        // Load developer mode update authenticity certificate.
        std::pair<std::vector<KCMItem>, ProvisionedStatusCode> load_developer_update_certificate();
        
        // Provisions devices with certificates using KCM storage. 
        struct PelionProvisioner
        {
            PelionProvisioner() {};
            ~PelionProvisioner();
            // Initialise the object.
            // This is here so we can handle errors on FCC/KCM init failure.
            ProvisionedStatusCode init();
            ProvisionedStatusCode store(const std::vector<KCMItem> &certificate);
            ProvisionedStatusCode get_provisioned_status();
        };
    };
};

#endif // PROVISIONING_H