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
#include <iostream>


enum ExitCode
{
    SUCCESS = 0,
    FAILURE,
    INCORRECT_ARGS
};


void usage()
{
    std::cout <<
R"(Usage

  pelion-provisioning-util [option] <file-path>
  pelion-provisioning-util [option]

The options here are mutually exclusive. 
If both options are given, the first valid one is used.

Options
  --kcm-item-store             = Store KCM items from a binary file.
  --get-pelion-status          = Get the Pelion status of the device.
    )";
}


ExitCode handle_store_command()
{
    PelionProvisioner provisioner;
    std::cout << "Provisioning device." << "\n";
    
    auto dev_cert_table = load_developer_cloud_credentials();
    auto dev_ret_val = provisioner.store(dev_cert_table);
    auto update_cert_table = load_developer_update_certificate();
    auto uc_ret_val = provisioner.store(update_cert_table);

    if (dev_ret_val != FCC_STATUS_SUCCESS)
    {
        std::cout << "Developer Certificate Provisioning failed. Error: " 
                  << fcc_get_fcc_error_string(dev_ret_val)
                  << "\n";
    }
    else
    {
        std::cout << "Developer Certificate Provisioning complete without error."
                  << "\n";
    }

    if (uc_ret_val != FCC_STATUS_SUCCESS)
    {
        std::cout << "Update Certificate Provisioning failed. Error: " 
                  << fcc_get_fcc_error_string(uc_ret_val)
                  << "\n";
    }
    else
    {
        std::cout << "Update Certificate Provisioning complete without error."
                  << "\n";
    }

    if (dev_ret_val != FCC_STATUS_SUCCESS || uc_ret_val != FCC_STATUS_SUCCESS)
        return FAILURE;
    return SUCCESS;
}


ExitCode handle_status_command()
{
    PelionProvisioner provisioner;
    std::cout << "Querying device status.. " << "\n";
    auto ret = provisioner.get_provisioned_status();

    if (ret != FCC_STATUS_SUCCESS)
    {
        std::cout << "Device is not configured correctly for Pelion Cloud!";
        return FAILURE;
    }
    else
    {
        std::cout << "Device is configured correctly. " 
                  << "You can connect to Pelion Cloud!"
                  << "\n";
        return SUCCESS;
    }
}


int main(int argc, char **argv)
{
    const std::string store_cmd = "--kcm-item-store";
    const std::string pelion_status_cmd = "--get-pelion-status";
    const std::string help = "--help";

    for(int i = 0; i < argc; i++)
    {
        if (argv[i] == store_cmd) 
        {
            return handle_store_command();
        }
        else if (argv[i] == pelion_status_cmd) 
        {
            return handle_status_command();
        }
        else if(argv[i] == help) 
        {
            usage();
            return SUCCESS;
        }
    }

    // If we reach this point the arguments were incorrect.    
    usage();

    return INCORRECT_ARGS;
}