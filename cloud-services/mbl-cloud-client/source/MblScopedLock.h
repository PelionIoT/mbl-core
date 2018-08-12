/*
 * Copyright (c) 2017 ARM Ltd.
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

#ifndef MblScopedLock_h_
#define MblScopedLock_h_

namespace mbl {

class MblMutex;

/**
 * Class to lock a mutex until the end of the scope.
 */
class MblScopedLock
{
public:
    /**
     * Create a scoped lock for the given mutex. The mutex will be locked until
     * this object's destructor has run.
     */
    explicit MblScopedLock(MblMutex& mutex);

    /**
     * Destructor - unlocks the mutex.
     */
    ~MblScopedLock();

private:
    MblMutex& mutex_;
};

} // namespace mbl

#endif // MblScopedLock_h_
