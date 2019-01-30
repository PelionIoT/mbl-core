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

#ifndef MblSdbusAdaptor_h_
#define MblSdbusAdaptor_h_

// Positive values for status, negative values for errors
typedef enum CCRBStatus
{
    //Errors

    //Success
    CCRB_STATUS_SUCCESS = 0,

    //Status
    CCRB_STATUS_IN_PROGRESS = 2,
} CCRBStatus;

typedef struct MblSdbusCallbacks
{
    int (*register_resources_callback)(const char *, CCRBStatus *);
    int (*deregister_resources_callback)(const char *, CCRBStatus *);
} MblSdbusCallbacks;


int32_t SdBusAdaptor_init(const MblSdbusCallbacks *callbacks);
int32_t SdBusAdaptor_deinit();
int32_t SdBusAdaptor_run();
int32_t SdBusAdaptor_stop();
int32_t SdBusAdaptor_attach_pipe_fd(int fd);

#endif // MblSdbusAdaptor_h_