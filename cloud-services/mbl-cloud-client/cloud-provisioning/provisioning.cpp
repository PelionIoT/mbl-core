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

#include "provisioning.h"
#include <fstream>
#include <iostream>


namespace mbl {
    namespace provisioning{
        // Path to the provisioning certificate directory
        static const std::string provisioning_cert_path{"/scratch/provisioning-certs/"};

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

        // Load a binary file to a vector of bytes
        std::pair<std::vector<uint8_t>, ProvisionedStatusCode> binary_file_to_bytes(const std::string &file_name)
        {
            const auto path = provisioning_cert_path + file_name;
            
            if (std::ifstream is{path, std::ios::binary | std::ios::ate}) 
            {
                auto pos = is.tellg();

                if (!is.good())
                {
                    std::cerr << "File with path " << path << "could not be opened" << "\n";
                    return std::make_pair(std::vector<uint8_t>{}, ProvisionedStatusCode::failure);
                }

                const auto size = static_cast<size_t>(pos);
                std::vector<uint8_t> output(size);
                is.seekg(0);

                if (!is.good())
                {
                    std::cerr << "File with path " << path << "could not be opened" << "\n";
                    return std::make_pair(std::vector<uint8_t>{}, ProvisionedStatusCode::failure);
                }

                if (is.read(reinterpret_cast<char*>(&output[0]), size)) 
                {
                    return std::make_pair(output, ProvisionedStatusCode::success);
                }
                else
                {
                    std::cerr << "File with path " << path << "could not be read" << "\n";
                   return std::make_pair(output, ProvisionedStatusCode::failure);
                }
            }
            else
            {
                std::cerr << "File with path " << path << "could not be opened" << "\n";
                return std::make_pair(std::vector<uint8_t>{}, ProvisionedStatusCode::failure);
            }  
        }

        std::pair<std::vector<KCMItem>, ProvisionedStatusCode> load_developer_update_certificate()
        {
            //Get the item data from files.
            const auto default_cert_pair = binary_file_to_bytes("arm_uc_default_certificate.bin");
            const auto vendor_id_pair = binary_file_to_bytes("arm_uc_vendor_id.bin");
            const auto class_id_pair = binary_file_to_bytes("arm_uc_class_id.bin");

            if (!(default_cert_pair.second == ProvisionedStatusCode::success
                  && vendor_id_pair.second == ProvisionedStatusCode::success
                  && class_id_pair.second == ProvisionedStatusCode::success))
            {
                std::cerr << "There was an error parsing the Update Credentials files" << "\n";
                return std::make_pair(std::vector<KCMItem>{}, ProvisionedStatusCode::failure);
            }
            else
            {
                std::vector<KCMItem> kcm_items {
                    KCMItem{g_fcc_update_authentication_certificate_name,
                            KCM_CERTIFICATE_ITEM,
                            default_cert_pair.first,
                            },
                    KCMItem{g_fcc_vendor_id_name,
                            KCM_CONFIG_ITEM,
                            vendor_id_pair.first,
                            },
                    KCMItem{g_fcc_class_id_name,
                            KCM_CONFIG_ITEM,
                            class_id_pair.first,
                            }
                };

                return std::make_pair(kcm_items, ProvisionedStatusCode::success);
            }
        }

        std::pair<std::vector<KCMItem>, ProvisionedStatusCode> load_developer_cloud_credentials()
        {
            //Get the item data from files.
            //Device general info
            const auto dev_bootstrap_endpoint_name                = binary_file_to_bytes("MBED_CLOUD_DEV_BOOTSTRAP_ENDPOINT_NAME.bin");
            //Bootstrap configuration
            const auto dev_bootstrap_device_certificate           = binary_file_to_bytes("MBED_CLOUD_DEV_BOOTSTRAP_DEVICE_CERTIFICATE.bin");
            const auto dev_bootstrap_server_root_ca_certificate   = binary_file_to_bytes("MBED_CLOUD_DEV_BOOTSTRAP_SERVER_ROOT_CA_CERTIFICATE.bin");
            const auto dev_bootstrap_device_private_key           = binary_file_to_bytes("MBED_CLOUD_DEV_BOOTSTRAP_DEVICE_PRIVATE_KEY.bin");
            const auto dev_bootstrap_server_uri                   = binary_file_to_bytes("MBED_CLOUD_DEV_BOOTSTRAP_SERVER_URI.bin");
            //device meta data
            const auto dev_manufacturer                           = binary_file_to_bytes("MBED_CLOUD_DEV_MANUFACTURER.bin");
            const auto dev_model_number                           = binary_file_to_bytes("MBED_CLOUD_DEV_MODEL_NUMBER.bin");
            const auto dev_serial_number                          = binary_file_to_bytes("MBED_CLOUD_DEV_SERIAL_NUMBER.bin");
            const auto dev_device_type                            = binary_file_to_bytes("MBED_CLOUD_DEV_DEVICE_TYPE.bin");
            const auto dev_hardware_version                       = binary_file_to_bytes("MBED_CLOUD_DEV_HARDWARE_VERSION.bin");
            const auto dev_memory_total_kb                        = binary_file_to_bytes("MBED_CLOUD_DEV_MEMORY_TOTAL_KB.bin");
            
            if (!(dev_bootstrap_endpoint_name.second == ProvisionedStatusCode::success
                  && dev_bootstrap_device_certificate.second == ProvisionedStatusCode::success
                  && dev_bootstrap_server_root_ca_certificate.second == ProvisionedStatusCode::success
                  && dev_bootstrap_device_private_key.second == ProvisionedStatusCode::success
                  && dev_bootstrap_server_uri.second == ProvisionedStatusCode::success
                  && dev_manufacturer.second == ProvisionedStatusCode::success
                  && dev_model_number.second == ProvisionedStatusCode::success
                  && dev_serial_number.second == ProvisionedStatusCode::success
                  && dev_device_type.second == ProvisionedStatusCode::success
                  && dev_hardware_version.second == ProvisionedStatusCode::success
                  && dev_memory_total_kb.second == ProvisionedStatusCode::success)
               )
            {
                std::cerr << "There was an error parsing the dev credentials files" << "\n";
                return std::make_pair(std::vector<KCMItem>{}, ProvisionedStatusCode::failure);
            }

            // Hack to make the useBootstrap parameter work
            // Convince the compiler the alignment is ok when casting to uint8_t
            uint32_t bs_flag_32 = 1;
            uint8_t* bs_flag_8_p = reinterpret_cast<uint8_t*>(&bs_flag_32);
            // Vector represents the 4 bytes of a uint32_t
            std::vector<uint8_t> bootstrap_mode_flag { bs_flag_8_p[0], bs_flag_8_p[1], bs_flag_8_p[2], bs_flag_8_p[3] };

            std::vector<KCMItem> kcm_items{
                //Device general info
                KCMItem{g_fcc_use_bootstrap_parameter_name,
                        KCM_CONFIG_ITEM,
                        bootstrap_mode_flag,
                        },
                KCMItem{g_fcc_endpoint_parameter_name,
                        KCM_CONFIG_ITEM,
                        dev_bootstrap_endpoint_name.first
                        },
                //Bootstrap configuration
                KCMItem{g_fcc_bootstrap_device_certificate_name,
                        KCM_CERTIFICATE_ITEM,
                        dev_bootstrap_device_certificate.first,
                        },
                KCMItem{g_fcc_bootstrap_server_ca_certificate_name,
                        KCM_CERTIFICATE_ITEM,
                        dev_bootstrap_server_root_ca_certificate.first,
                        },
                KCMItem{g_fcc_bootstrap_device_private_key_name,
                        KCM_PRIVATE_KEY_ITEM,
                        dev_bootstrap_device_private_key.first,
                        },
                KCMItem{g_fcc_bootstrap_server_uri_name,
                        KCM_CONFIG_ITEM,
                        dev_bootstrap_server_uri.first
                        },
                //device meta data
                KCMItem{g_fcc_manufacturer_parameter_name,
                        KCM_CONFIG_ITEM,
                        dev_manufacturer.first
                        },
                KCMItem{g_fcc_model_number_parameter_name,
                        KCM_CONFIG_ITEM,
                        dev_model_number.first
                        },
                KCMItem{g_fcc_device_serial_number_parameter_name,
                        KCM_CONFIG_ITEM,
                        dev_serial_number.first,
                        },
                KCMItem{g_fcc_device_type_parameter_name,
                        KCM_CONFIG_ITEM,
                        dev_device_type.first
                        },
                KCMItem{g_fcc_hardware_version_parameter_name,
                        KCM_CONFIG_ITEM,
                        dev_hardware_version.first
                        },
                KCMItem{g_fcc_memory_size_parameter_name,
                        KCM_CONFIG_ITEM,
                        dev_memory_total_kb.first
                        }
            };

            return std::make_pair(kcm_items, ProvisionedStatusCode::success);
        }

        ProvisionedStatusCode PelionProvisioner::init()
        {
            // Initialise FCC
            const auto fcc_status = fcc_init();

            if (fcc_status != FCC_STATUS_SUCCESS)
            {
                print_fcc_error_status(std::cerr, "FCC init failed!", fcc_status);
                return ProvisionedStatusCode::failure;
            }

            return ProvisionedStatusCode::success;
        }

        PelionProvisioner::~PelionProvisioner()
        {
            // fcc_finalize also finalises kcm.
            const auto ret = fcc_finalize();

            if (ret != FCC_STATUS_SUCCESS)
                print_fcc_error_status(std::cerr, "FCC finalise failed!", ret);
        }

        ProvisionedStatusCode PelionProvisioner::store(const std::vector<KCMItem> &certificate)
        {
            for(const auto &item: certificate)
            {  
                const auto kcm_delete_status = kcm_item_delete(reinterpret_cast<const uint8_t*>(item.name.c_str()),
                                                               item.name.size(),
                                                               item.type);

                if (kcm_delete_status != KCM_STATUS_SUCCESS && kcm_delete_status != KCM_STATUS_ITEM_NOT_FOUND)
                {
                    print_kcm_error_status(std::cerr,
                                           "Failed to delete KCM Item! Item name: " + item.name,
                                           kcm_delete_status);
                    return ProvisionedStatusCode::failure;
                }

                const auto kcm_store_status = kcm_item_store(reinterpret_cast<const uint8_t*>(item.name.c_str()),
                                                             item.name.size(),
                                                             item.type,
                                                             true, // is_factory flag
                                                             item.data_blob.data(),
                                                             item.data_blob.size(),
                                                             NULL);

                if (kcm_store_status != KCM_STATUS_SUCCESS)
                {
                    print_kcm_error_status(std::cerr,
                                           "Failed to store KCM Item! Item name: " + item.name,
                                           kcm_store_status);
                    return ProvisionedStatusCode::failure;
                }
            }

            return ProvisionedStatusCode::success;
        }

        ProvisionedStatusCode PelionProvisioner::get_provisioned_status()
        {
            const auto ret = fcc_verify_device_configured_4mbed_cloud();

            if (ret != FCC_STATUS_SUCCESS)
            {
                const auto output_info = fcc_get_error_and_warning_data();

                if (output_info && output_info->error_string_info)
                {
                    std::cerr << output_info->error_string_info << "\n";
                }

                return ProvisionedStatusCode::failure;
            }
            else
            {
                return ProvisionedStatusCode::success;
            }
        }
    };
};