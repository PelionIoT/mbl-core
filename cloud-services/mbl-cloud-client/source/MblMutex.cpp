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

#include "MblMutex.h"

#include <cassert>

namespace mbl
{

MblMutex::MblMutex()
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
    const int ret = pthread_mutex_init(&mutex_, &attr);
    assert(ret == 0);
}

MblMutex::~MblMutex()
{
    const int ret = pthread_mutex_destroy(&mutex_);
    assert(ret == 0);
}

void MblMutex::lock()
{
    const int ret = pthread_mutex_lock(&mutex_);
    assert(ret == 0);
}

void MblMutex::unlock()
{
    const int ret = pthread_mutex_unlock(&mutex_);
    assert(ret == 0);
}

bool MblMutex::try_lock()
{
    const int ret = pthread_mutex_trylock(&mutex_);
    return ret == 0;
}

} // namespace mbl
