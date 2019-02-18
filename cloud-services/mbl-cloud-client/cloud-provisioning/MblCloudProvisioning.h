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


#include "factory_configurator_client.h"
#include "key_config_manager.h"
#include "fcc_defs.h"
#include "fcc_utils.h"
#include <vector>
#include <string>
#include <iostream>


struct KCMItem
{
    const std::string           name;
    const kcm_item_type_e       type;
    const std::vector<uint8_t>  data_blob;
    const size_t                data_size;
};


//  Loads a binary file containing mcc data into a byte array.
std::vector<uint8_t>    provisioning_file_to_array(const std::string &file_name);
std::string             provisioning_file_to_string(const std::string &file_name);

std::vector<KCMItem>    load_developer_cloud_credentials();
std::vector<KCMItem>    load_developer_update_certificate();


class PelionProvisioner
{
public:
    PelionProvisioner();
    ~PelionProvisioner();

    fcc_status_e get_stored_data(const std::vector<KCMItem> &certificate);
    fcc_status_e store(const std::vector<KCMItem> &certificate);
    fcc_status_e get_provisioned_status();

private:
    // bootstrap mode flag for KCM. 
    // We're always using bootstrap mode, so this is const.
    const uint32_t is_bootstrap_mode{1};
};
