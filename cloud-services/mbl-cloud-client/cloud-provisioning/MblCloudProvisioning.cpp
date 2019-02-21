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

namespace mbl {
    namespace provisioning{
        // Path to the provisioning certificate directory
        static const std::string provisioning_cert_path{"/scratch/provisioning-certs/"};

        // Check a string ends with a substring
        static bool endswith(const std::string &full_string, const std::string &ending)
        {
            if (full_string.length() >= ending.length()) {
                return (0 == full_string.compare(full_string.length() - ending.length(),
                                                ending.length(),
                                                ending));
            } else {
                return false;
            }
        }

        void print_fcc_error_status(std::ostream &out_stream,
                                    const std::string &error_msg,
                                    const fcc_status_e status)
        {
            out_stream << error_msg << "\n"
                       << "Error status is: "
                       << fcc_get_fcc_error_string(status)
                       << "\n";
        }

        void print_kcm_error_status(std::ostream &out_stream,
                                    const std::string &error_msg,
                                    const kcm_status_e status)
        {
            out_stream << error_msg << "\n"
                       << "Error status is: "
                       << fcc_get_kcm_error_string(status)
                       << "\n";
        }

        std::string provisioning_file_to_string(const std::string &file_name)
        {
            const auto path = provisioning_cert_path + file_name;

            std::ifstream infile(path, std::ios_base::binary);
            std::string line;
            std::getline(infile, line);

            return line;
        }

        std::vector<uint8_t> provisioning_file_to_bytes(const std::string &file_name)
        {
            const auto path = provisioning_cert_path + file_name;

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
            const auto arm_uc_default_certificate = provisioning_file_to_bytes("arm_uc_default_certificate.bin");
            const auto arm_uc_vendor_id = provisioning_file_to_bytes("arm_uc_vendor_id.bin");
            const auto arm_uc_class_id = provisioning_file_to_bytes("arm_uc_class_id.bin");

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
            const auto mbed_cloud_dev_bootstrap_device_certificate           = provisioning_file_to_bytes("MBED_CLOUD_DEV_BOOTSTRAP_DEVICE_CERTIFICATE.bin");
            const auto mbed_cloud_dev_bootstrap_server_root_ca_certificate   = provisioning_file_to_bytes("MBED_CLOUD_DEV_BOOTSTRAP_SERVER_ROOT_CA_CERTIFICATE.bin");
            const auto mbed_cloud_dev_bootstrap_device_private_key           = provisioning_file_to_bytes("MBED_CLOUD_DEV_BOOTSTRAP_DEVICE_PRIVATE_KEY.bin");
            const auto mbed_cloud_dev_bootstrap_server_uri                   = provisioning_file_to_string("MBED_CLOUD_DEV_BOOTSTRAP_SERVER_URI.bin");
            //device meta data
            const auto mbed_cloud_dev_manufacturer                           = provisioning_file_to_string("MBED_CLOUD_DEV_MANUFACTURER.bin");
            const auto mbed_cloud_dev_model_number                           = provisioning_file_to_string("MBED_CLOUD_DEV_MODEL_NUMBER.bin");
            const auto mbed_cloud_dev_serial_number                          = provisioning_file_to_string("MBED_CLOUD_DEV_SERIAL_NUMBER.bin");
            const auto mbed_cloud_dev_device_type                            = provisioning_file_to_string("MBED_CLOUD_DEV_DEVICE_TYPE.bin");
            const auto mbed_cloud_dev_hardware_version                       = provisioning_file_to_string("MBED_CLOUD_DEV_HARDWARE_VERSION.bin");
            const auto mbed_cloud_dev_memory_total_kb                        = provisioning_file_to_string("MBED_CLOUD_DEV_MEMORY_TOTAL_KB.bin");

            // Hack to make the useBootstrap parameter work
            uint32_t is_bootstrap_mode = 1;
            std::vector<uint8_t> bootstrap_mode_flag(sizeof(uint32_t));
            bootstrap_mode_flag[0] = *reinterpret_cast<uint8_t*>(&is_bootstrap_mode);

            std::vector<KCMItem> kcm_items{
                //Device general info
                KCMItem{g_fcc_use_bootstrap_parameter_name,
                        KCM_CONFIG_ITEM,
                        bootstrap_mode_flag,
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

        ProvisionedStatusCode PelionProvisioner::init()
        {
            // Initialise FCC
            const auto fcc_status = fcc_init();
            // ensure KCM is initialised
            const auto kcm_status = kcm_init();

            if (fcc_status != FCC_STATUS_SUCCESS)
            {
                print_fcc_error_status(std::cout, "FCC init failed!", fcc_status);
                return FAILURE;
            }

            if (kcm_status != KCM_STATUS_SUCCESS)
            {
                print_kcm_error_status(std::cout, "KCM init failed!", kcm_status);
                return FAILURE;
            }

            return SUCCESS;
        }

        PelionProvisioner::~PelionProvisioner()
        {
            // fcc_finalise also finalises kcm...
            const auto ret = fcc_finalize();

            if (ret != FCC_STATUS_SUCCESS)
                print_fcc_error_status(std::cout, "FCC finalise failed!", ret);
        }

        ProvisionedStatusCode PelionProvisioner::store(const std::vector<KCMItem> &certificate)
        {
            for(const auto &item: certificate)
            {  
                const auto kcm_status = kcm_item_store((const uint8_t*)(item.name.c_str()),
                                                       item.name.size(),
                                                       item.type,
                                                       true,
                                                       (const uint8_t*)(item.data_blob.data()),
                                                       item.data_blob.size(),
                                                       NULL);

                if (kcm_status != KCM_STATUS_SUCCESS)
                {
                    print_kcm_error_status(std::cout,
                                           "Failed to store KCM Item! Item name: " + item.name,
                                           kcm_status);
                    return FAILURE;
                }
            }

            return SUCCESS;
        }

        ProvisionedStatusCode PelionProvisioner::get_provisioned_status()
        {
            const auto ret = fcc_verify_device_configured_4mbed_cloud();

            if (ret != FCC_STATUS_SUCCESS)
            {
                const auto output_info = fcc_get_error_and_warning_data();

                if (output_info && output_info->error_string_info)
                {
                    std::cout << output_info->error_string_info << "\n";
                }
                
                return FAILURE;
            }
            else
            {
                return SUCCESS;
            }
        }
    };
};