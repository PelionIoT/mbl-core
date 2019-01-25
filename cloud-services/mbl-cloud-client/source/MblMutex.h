/*
 * Copyright (c) 2017 Arm Limited and Contributors. All rights reserved.
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

#ifndef MblMutex_h_
#define MblMutex_h_

#include <pthread.h>

namespace mbl {

/**
 * A non-recursive mutex based on pthread_mutex_t
 */
class MblMutex
{
public:
    MblMutex();
    ~MblMutex();

    void lock();
    void unlock();
    bool try_lock();

private:
    // No copying
    MblMutex(const MblMutex&);
    MblMutex& operator=(const MblMutex&);

    pthread_mutex_t mutex_;
};

} // namespace mbl

#endif // MblMutex_h_
