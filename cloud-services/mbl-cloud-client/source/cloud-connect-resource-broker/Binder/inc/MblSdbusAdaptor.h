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




typedef struct MblSdbusCallbacks
{
     int (*register_resources_callback)(const char *);
}MblSdbusCallbacks;


int32_t SdBusAdaptor_init(const MblSdbusCallbacks *callbacks);
int32_t SdBusAdaptor_finalize();
int32_t SdBusAdaptor_run();
int32_t SdBusAdaptor_stop();

#endif // MblSdbusAdaptor_h_