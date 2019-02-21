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

#include "MblCloudProvisioning.h"
#include <fstream>
#include <iostream>



// Check if a string ends with a substring
bool endswith(const std::string &fullString,  const std::string &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare(fullString.length() - ending.length(),
                                        ending.length(),
                                        ending));
    } else {
        return false;
    }
}

// Load a file to a string
std::string provisioning_file_to_string(const std::string &file_name)
{
    std::string path{"/scratch/provisioning-certs/"};
    path.append(file_name);

    std::ifstream infile(path, std::ios_base::binary);
    std::string line;
    std::getline(infile, line);

    return line;
}


//  Loads a file with multiline text into a byte array.
std::vector<uint8_t> provisioning_file_to_array(const std::string &fileName)
{
    std::string path{"/scratch/provisioning-certs/"};
    path.append(fileName);
    
    std::ifstream infile(path, std::ios_base::binary);
    std::string line;
    std::vector<uint8_t> output;

    while (std::getline(infile, line))
    {
        if (endswith(line, "\n") || endswith(line, "\r"))
            line.pop_back();
        const auto numeric = (uint8_t)strtoul(line.c_str(), nullptr, 16);
        output.push_back(numeric);
    };

    return output;
}


std::vector<KCMItem> load_developer_update_certificate()
{
    //Get the item data from files.
    const auto arm_uc_default_certificate = provisioning_file_to_array("arm_uc_default_certificate.bin");
    const auto arm_uc_vendor_id = provisioning_file_to_array("arm_uc_vendor_id.bin");
    const auto arm_uc_class_id = provisioning_file_to_array("arm_uc_class_id.bin");

    std::vector<KCMItem> kcm_items {
        KCMItem{g_fcc_update_authentication_certificate_name,
                KCM_CERTIFICATE_ITEM,
                arm_uc_default_certificate,
                },
        KCMItem{g_fcc_vendor_id_name,
                KCM_CONFIG_ITEM,
                arm_uc_vendor_id,
                },
        KCMItem{g_fcc_class_id_name,
                KCM_CONFIG_ITEM,
                arm_uc_class_id,
                }
    };

    return kcm_items;
}

std::vector<KCMItem> load_developer_cloud_credentials()
{
    //Get the item data from files.
    //Device general info
    const auto mbed_cloud_dev_bootstrap_endpoint_name                = provisioning_file_to_string("MBED_CLOUD_DEV_BOOTSTRAP_ENDPOINT_NAME.bin");
    //Bootstrap configuration
    const auto mbed_cloud_dev_bootstrap_device_certificate           = provisioning_file_to_array("MBED_CLOUD_DEV_BOOTSTRAP_DEVICE_CERTIFICATE.bin");
    const auto mbed_cloud_dev_bootstrap_server_root_ca_certificate   = provisioning_file_to_array("MBED_CLOUD_DEV_BOOTSTRAP_SERVER_ROOT_CA_CERTIFICATE.bin");
    const auto mbed_cloud_dev_bootstrap_device_private_key           = provisioning_file_to_array("MBED_CLOUD_DEV_BOOTSTRAP_DEVICE_PRIVATE_KEY.bin");
    const auto mbed_cloud_dev_bootstrap_server_uri                   = provisioning_file_to_string("MBED_CLOUD_DEV_BOOTSTRAP_SERVER_URI.bin");
    //device meta data
    const auto mbed_cloud_dev_manufacturer                           = provisioning_file_to_string("MBED_CLOUD_DEV_MANUFACTURER.bin");
    const auto mbed_cloud_dev_model_number                           = provisioning_file_to_string("MBED_CLOUD_DEV_MODEL_NUMBER.bin");
    const auto mbed_cloud_dev_serial_number                          = provisioning_file_to_string("MBED_CLOUD_DEV_SERIAL_NUMBER.bin");
    const auto mbed_cloud_dev_device_type                            = provisioning_file_to_string("MBED_CLOUD_DEV_DEVICE_TYPE.bin");
    const auto mbed_cloud_dev_hardware_version                       = provisioning_file_to_string("MBED_CLOUD_DEV_HARDWARE_VERSION.bin");
    const auto mbed_cloud_dev_memory_total_kb                        = provisioning_file_to_string("MBED_CLOUD_DEV_MEMORY_TOTAL_KB.bin");


    std::vector<KCMItem> kcm_items{
        //Device general info
        KCMItem{g_fcc_use_bootstrap_parameter_name,
                KCM_CONFIG_ITEM,
                std::vector<uint8_t>{ 1 },
                },
        KCMItem{g_fcc_endpoint_parameter_name,
                KCM_CONFIG_ITEM,
                std::vector<uint8_t>{ mbed_cloud_dev_bootstrap_endpoint_name.begin(),
                                      mbed_cloud_dev_bootstrap_endpoint_name.end() },
                },
        //Bootstrap configuration
        KCMItem{g_fcc_bootstrap_device_certificate_name,
                KCM_CERTIFICATE_ITEM,
                mbed_cloud_dev_bootstrap_device_certificate,
                },
        KCMItem{g_fcc_bootstrap_server_ca_certificate_name,
                KCM_CERTIFICATE_ITEM,
                mbed_cloud_dev_bootstrap_server_root_ca_certificate,
                },
        KCMItem{g_fcc_bootstrap_device_private_key_name,
                KCM_PRIVATE_KEY_ITEM,
                mbed_cloud_dev_bootstrap_device_private_key,
                },
        KCMItem{g_fcc_bootstrap_server_uri_name,
                KCM_CONFIG_ITEM,
                std::vector<uint8_t>{ mbed_cloud_dev_bootstrap_server_uri.begin(),
                                      mbed_cloud_dev_bootstrap_server_uri.end() },
                },
        //device meta data
        KCMItem{g_fcc_manufacturer_parameter_name,
                KCM_CONFIG_ITEM,
                std::vector<uint8_t>{ mbed_cloud_dev_manufacturer.begin(),
                                      mbed_cloud_dev_manufacturer.end() },
                },
        KCMItem{g_fcc_model_number_parameter_name,
                KCM_CONFIG_ITEM,
                std::vector<uint8_t>{ mbed_cloud_dev_model_number.begin(),
                                      mbed_cloud_dev_model_number.end() },
                },
        KCMItem{g_fcc_device_serial_number_parameter_name,
                KCM_CONFIG_ITEM,
                std::vector<uint8_t>{ mbed_cloud_dev_serial_number.begin(),
                                      mbed_cloud_dev_serial_number.end() },
                },
        KCMItem{g_fcc_device_type_parameter_name,
                KCM_CONFIG_ITEM,
                std::vector<uint8_t>{ mbed_cloud_dev_device_type.begin(),
                                      mbed_cloud_dev_device_type.end() },
                },
        KCMItem{g_fcc_hardware_version_parameter_name,
                KCM_CONFIG_ITEM,
                std::vector<uint8_t>{ mbed_cloud_dev_hardware_version.begin(),
                                      mbed_cloud_dev_hardware_version.end() },
                },
        KCMItem{g_fcc_memory_size_parameter_name,
                KCM_CONFIG_ITEM,
                std::vector<uint8_t>{ mbed_cloud_dev_memory_total_kb.begin(),
                                      mbed_cloud_dev_memory_total_kb.end() },
                }
    };

    return kcm_items;
}

PelionProvisioner::PelionProvisioner()
{
    const auto fcc_status = fcc_init();
    // ensure KCM is initialised
    const auto kcm_status = kcm_init();
    if (fcc_status != FCC_STATUS_SUCCESS)
        std::cout << "FCC init failed! Error: " 
                  << fcc_get_fcc_error_string(fcc_status)
                  << "\n"; 

    if (kcm_status != KCM_STATUS_SUCCESS)
        std::cout << "KCM init failed! Error: " 
                  << fcc_get_kcm_error_string(kcm_status)
                  << "\n"; 
}

PelionProvisioner::~PelionProvisioner()
{
    // fcc_finalise also finalises kcm...
    const auto ret = fcc_finalize();
    if (ret != FCC_STATUS_SUCCESS)
        std::cout << "FCC finalise failed! Error: "
                  << fcc_get_fcc_error_string(ret)
                  << "\n"; 
}

fcc_status_e PelionProvisioner::store(const std::vector<KCMItem> &certificate)
{
    kcm_status_e kcm_status;

    for(const auto &item: certificate)
    {  
        if (item.name == g_fcc_use_bootstrap_parameter_name)
        {
            kcm_status = kcm_item_store((const uint8_t*)(item.name.c_str()),
                                        item.name.size(),
                                        item.type,
                                        true,
                                        (const uint8_t*)&is_bootstrap_mode,
                                        sizeof(uint32_t),
                                        NULL);
        }
        else
        {
            kcm_status = kcm_item_store((const uint8_t*)(item.name.c_str()),
                                        item.name.size(),
                                        item.type,
                                        true,
                                        (const uint8_t*)(item.data_blob.data()),
                                        item.data_blob.size(),
                                        NULL);
        } 

        if (kcm_status != KCM_STATUS_SUCCESS)
        {
            std::cout << "Failed to store KCM Item! Item name: "
                      << item.name << "\n" << "KCM error: "
                      << fcc_get_kcm_error_string(kcm_status) << "\n";
        }
    }

    return fcc_verify_device_configured_4mbed_cloud();
}

fcc_status_e PelionProvisioner::get_provisioned_status()
{
    const auto ret = fcc_verify_device_configured_4mbed_cloud();
    if (ret != FCC_STATUS_SUCCESS)
    {
        std::cout << "Error: "
                  << fcc_get_fcc_error_string(ret)
                  << "\n";
    }
    return ret;
}
