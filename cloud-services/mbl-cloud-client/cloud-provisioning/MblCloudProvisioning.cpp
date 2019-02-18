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

// Load a provisioning cert file to a string
std::string provisioning_file_to_string(const std::string &file_name)
{
    std::string path{"/scratch/provisioning-certs/"};
    path.append(file_name);

    std::ifstream infile(path, std::ios_base::binary);
    std::string line;
    std::getline(infile, line);

    return line;
}


//  Loads a file containing mcc data into a byte array.
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
                arm_uc_default_certificate.size()
                },
        KCMItem{g_fcc_vendor_id_name,
                KCM_CONFIG_ITEM,
                arm_uc_vendor_id,
                arm_uc_vendor_id.size()
                },
        KCMItem{g_fcc_class_id_name,
                KCM_CONFIG_ITEM,
                arm_uc_class_id,
                arm_uc_class_id.size()
                }
    };

    return kcm_items;
}

std::vector<KCMItem> load_developer_cloud_credentials()
{
    //Get the item data from files.
    //Device general info
    const auto MBED_CLOUD_DEV_BOOTSTRAP_ENDPOINT_NAME                = provisioning_file_to_string("MBED_CLOUD_DEV_BOOTSTRAP_ENDPOINT_NAME.bin");
    //Bootstrap configuration
    const auto MBED_CLOUD_DEV_BOOTSTRAP_DEVICE_CERTIFICATE           = provisioning_file_to_array("MBED_CLOUD_DEV_BOOTSTRAP_DEVICE_CERTIFICATE.bin");
    const auto MBED_CLOUD_DEV_BOOTSTRAP_SERVER_ROOT_CA_CERTIFICATE   = provisioning_file_to_array("MBED_CLOUD_DEV_BOOTSTRAP_SERVER_ROOT_CA_CERTIFICATE.bin");
    const auto MBED_CLOUD_DEV_BOOTSTRAP_DEVICE_PRIVATE_KEY           = provisioning_file_to_array("MBED_CLOUD_DEV_BOOTSTRAP_DEVICE_PRIVATE_KEY.bin");
    const auto MBED_CLOUD_DEV_BOOTSTRAP_SERVER_URI                   = provisioning_file_to_string("MBED_CLOUD_DEV_BOOTSTRAP_SERVER_URI.bin");
    //device meta data
    const auto MBED_CLOUD_DEV_MANUFACTURER                           = provisioning_file_to_string("MBED_CLOUD_DEV_MANUFACTURER.bin");
    const auto MBED_CLOUD_DEV_MODEL_NUMBER                           = provisioning_file_to_string("MBED_CLOUD_DEV_MODEL_NUMBER.bin");
    const auto MBED_CLOUD_DEV_SERIAL_NUMBER                          = provisioning_file_to_string("MBED_CLOUD_DEV_SERIAL_NUMBER.bin");
    const auto MBED_CLOUD_DEV_DEVICE_TYPE                            = provisioning_file_to_string("MBED_CLOUD_DEV_DEVICE_TYPE.bin");
    const auto MBED_CLOUD_DEV_HARDWARE_VERSION                       = provisioning_file_to_string("MBED_CLOUD_DEV_HARDWARE_VERSION.bin");
    const auto MBED_CLOUD_DEV_MEMORY_TOTAL_KB                        = provisioning_file_to_string("MBED_CLOUD_DEV_MEMORY_TOTAL_KB.bin");


    std::vector<KCMItem> kcm_items{
        //Device general info
        KCMItem{g_fcc_use_bootstrap_parameter_name,
                KCM_CONFIG_ITEM,
                std::vector<uint8_t>{ 1 },
                sizeof(uint32_t)
                },
        KCMItem{g_fcc_endpoint_parameter_name,
                KCM_CONFIG_ITEM,
                std::vector<uint8_t>{ MBED_CLOUD_DEV_BOOTSTRAP_ENDPOINT_NAME.begin(),
                                      MBED_CLOUD_DEV_BOOTSTRAP_ENDPOINT_NAME.end() },
                MBED_CLOUD_DEV_BOOTSTRAP_ENDPOINT_NAME.size() 
                },
        //Bootstrap configuration
        KCMItem{g_fcc_bootstrap_device_certificate_name,
                KCM_CERTIFICATE_ITEM,
                MBED_CLOUD_DEV_BOOTSTRAP_DEVICE_CERTIFICATE,
                MBED_CLOUD_DEV_BOOTSTRAP_DEVICE_CERTIFICATE.size()
                },
        KCMItem{g_fcc_bootstrap_server_ca_certificate_name,
                KCM_CERTIFICATE_ITEM,
                MBED_CLOUD_DEV_BOOTSTRAP_SERVER_ROOT_CA_CERTIFICATE,
                MBED_CLOUD_DEV_BOOTSTRAP_SERVER_ROOT_CA_CERTIFICATE.size()
                },
        KCMItem{g_fcc_bootstrap_device_private_key_name,
                KCM_PRIVATE_KEY_ITEM,
                MBED_CLOUD_DEV_BOOTSTRAP_DEVICE_PRIVATE_KEY,
                MBED_CLOUD_DEV_BOOTSTRAP_DEVICE_PRIVATE_KEY.size()
                },
        KCMItem{g_fcc_bootstrap_server_uri_name,
                KCM_CONFIG_ITEM,
                std::vector<uint8_t>{ MBED_CLOUD_DEV_BOOTSTRAP_SERVER_URI.begin(),
                                      MBED_CLOUD_DEV_BOOTSTRAP_SERVER_URI.end() },
                MBED_CLOUD_DEV_BOOTSTRAP_SERVER_URI.size()
                },
        //device meta data
        KCMItem{g_fcc_manufacturer_parameter_name,
                KCM_CONFIG_ITEM,
                std::vector<uint8_t>{ MBED_CLOUD_DEV_MANUFACTURER.begin(),
                                      MBED_CLOUD_DEV_MANUFACTURER.end() },
                MBED_CLOUD_DEV_MANUFACTURER.size()
                },
        KCMItem{g_fcc_model_number_parameter_name,
                KCM_CONFIG_ITEM,
                std::vector<uint8_t>{ MBED_CLOUD_DEV_MODEL_NUMBER.begin(),
                                      MBED_CLOUD_DEV_MODEL_NUMBER.end() },
                MBED_CLOUD_DEV_MODEL_NUMBER.size()
                },
        KCMItem{g_fcc_device_serial_number_parameter_name,
                KCM_CONFIG_ITEM,
                std::vector<uint8_t>{ MBED_CLOUD_DEV_SERIAL_NUMBER.begin(),
                                      MBED_CLOUD_DEV_SERIAL_NUMBER.end() },
                MBED_CLOUD_DEV_SERIAL_NUMBER.size()
                },
        KCMItem{g_fcc_device_type_parameter_name,
                KCM_CONFIG_ITEM,
                std::vector<uint8_t>{ MBED_CLOUD_DEV_DEVICE_TYPE.begin(),
                                      MBED_CLOUD_DEV_DEVICE_TYPE.end() },
                MBED_CLOUD_DEV_DEVICE_TYPE.size()
                },
        KCMItem{g_fcc_hardware_version_parameter_name,
                KCM_CONFIG_ITEM,
                std::vector<uint8_t>{ MBED_CLOUD_DEV_HARDWARE_VERSION.begin(),
                                      MBED_CLOUD_DEV_HARDWARE_VERSION.end() },
                MBED_CLOUD_DEV_HARDWARE_VERSION.size()
                },
        KCMItem{g_fcc_memory_size_parameter_name,
                KCM_CONFIG_ITEM,
                std::vector<uint8_t>{ MBED_CLOUD_DEV_MEMORY_TOTAL_KB.begin(),
                                      MBED_CLOUD_DEV_MEMORY_TOTAL_KB.end() },
                MBED_CLOUD_DEV_MEMORY_TOTAL_KB.size()
                }
    };

    return kcm_items;
}

PelionProvisioner::PelionProvisioner()
{
    const auto ret = fcc_init();
    // ensure KCM is initialised
    kcm_init();
    if (ret != FCC_STATUS_SUCCESS)
        std::cout << "FCC init failed! Error: " 
                  << fcc_get_fcc_error_string(ret)
                  << "\n"; 
}

PelionProvisioner::~PelionProvisioner()
{
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
                                        strlen(item.name.c_str()),
                                        item.type,
                                        true,
                                        (const uint8_t*)&is_bootstrap_mode,
                                        item.data_size,
                                        NULL);
        } 
        else
        {
            kcm_status = kcm_item_store((const uint8_t*)(item.name.c_str()),
                                        strlen(item.name.c_str()),
                                        item.type,
                                        true,
                                        (const uint8_t*)(item.data_blob.data()),
                                        item.data_size,
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
