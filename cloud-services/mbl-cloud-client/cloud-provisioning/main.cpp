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


enum class ExitCode
{
    success = 0,
    failure = 1,
    incorrect_args = 2
};


void usage()
{
    std::cout <<
R"(Usage:
  pelion-provisioning-util [option]


Options:
  --kcm-item-store               Store KCM items from a binary file.
  --get-pelion-status            Get the Pelion status of the device.
  --help                         Show this message and exit.
    )";
}


ExitCode handle_store_command()
{
    mbl::provisioning::PelionProvisioner provisioner;
    const auto init_status = provisioner.init();
    
    if (init_status != mbl::provisioning::ProvisionedStatusCode::success)
        return ExitCode::failure;

    std::cout << "Provisioning device." << "\n";
    
    const auto dev_cert_table = mbl::provisioning::load_developer_cloud_credentials();
    const auto update_cert_table = mbl::provisioning::load_developer_update_certificate();

    if (!(dev_cert_table.second == mbl::provisioning::ProvisionedStatusCode::success
          && update_cert_table.second == mbl::provisioning::ProvisionedStatusCode::success))
        return ExitCode::failure;
    
    const auto dev_ret_val = provisioner.store(dev_cert_table.first);
    const auto uc_ret_val = provisioner.store(update_cert_table.first);

    if (dev_ret_val != mbl::provisioning::ProvisionedStatusCode::success)
    {
        std::cerr << "Developer Certificate Provisioning failed."
                  << "\n";
        return ExitCode::failure;
    }

    if (uc_ret_val != mbl::provisioning::ProvisionedStatusCode::success)
    {
        std::cerr << "Update Certificate Provisioning failed."
                  << "\n";
        return ExitCode::failure;
    }

    std::cout << "Provisioning process completed without error." << "\n";
    return ExitCode::success;
}


ExitCode handle_status_command()
{
    mbl::provisioning::PelionProvisioner provisioner;
    const auto init_status = provisioner.init();
    
    if (init_status != mbl::provisioning::ProvisionedStatusCode::success)
        return ExitCode::failure;
    
    std::cout << "Querying device status... " << "\n";
    const auto ret = provisioner.get_provisioned_status();

    if (ret != mbl::provisioning::ProvisionedStatusCode::success)
    {
        return ExitCode::failure;
    }
    else
    {
        std::cout << "Device is configured correctly. " 
                  << "You can connect to Pelion Cloud!"
                  << "\n";
        return ExitCode::success;
    }
}


int main(int argc, char **argv)
{
    static const std::string store_cmd = "--kcm-item-store";
    static const std::string pelion_status_cmd = "--get-pelion-status";
    static const std::string help = "--help";

    if (argc > 2)
    {
        usage();
        return static_cast<int>(ExitCode::incorrect_args);
    }
    
    for(int i = 0; i < argc; i++)
    {
        if (argv[i] == store_cmd) 
        {
            return static_cast<int>(handle_store_command());
        }
        else if (argv[i] == pelion_status_cmd) 
        {
            return static_cast<int>(handle_status_command());
        }
        else if(argv[i] == help) 
        {
            usage();
            return static_cast<int>(ExitCode::success);
        }
    }

    // If we reach this point the arguments were incorrect.    
    usage();
    return static_cast<int>(ExitCode::incorrect_args);
}