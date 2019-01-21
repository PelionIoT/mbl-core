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


#ifndef MblCloudConnectResourceBroker_h_
#define MblCloudConnectResourceBroker_h_

#include "MblCloudConnectIpcInterface.h"

#include  <memory>

namespace mbl {

class MblCloudConnectResourceBroker {

public:

    // Currently, this constructor is called from MblCloudClient thread.
    // Might change in future. If we shall need to call the constructor from the
    // ccrb thread, change MblCloudClient::cloud_connect_resource_broker_ instance
    // member to be a pointer and call the constructor from ccrb thread thread_function
    MblCloudConnectResourceBroker();
    ~MblCloudConnectResourceBroker();

    // initialize ccrb instance
    MblError init();

    // run ccrb
    MblError run();


    // The caller thread will join with the IPC thread
    // param[in] args - output parameter that will contain thread output data
    // return Error::None if succeeded, Error::ThreadJoiningFailed otherwise
    MblError thread_join(void **args);

    // Signals to the IPC thread that it should finish ASAP
    // return Error::None if succeeded, Error::ThreadFinishingFailed otherwise
    MblError thread_finish();

    // Thread main function.
    static void *thread_function(void *ccrb_instance_ptr);

private:

    std::unique_ptr<MblCloudConnectIpcInterface> ipc_;
};

} // namespace mbl

#endif // MblCloudConnectResourceBroker_h_
