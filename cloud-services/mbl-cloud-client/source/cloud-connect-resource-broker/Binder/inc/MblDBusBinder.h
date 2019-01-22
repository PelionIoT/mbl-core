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


#ifndef MblDBusBinder_h_
#define MblDBusBinder_h_

#include "MblError.h"
#include <stdint.h>
// FIXME : remove later
#define tr_info(s)
#define tr_debug(s)

namespace mbl {

/*! \file MblDBusBinder.h
 *  \brief MblDBusBinder.
 *  This class provides an binding and abstraction interface for a D-Bus IPC.
*/
class MblDBusBinder {

public:

    MblDBusBinder() = default;
    virtual ~MblDBusBinder() = default;
    
    virtual MblError Init() = 0;
    virtual MblError Finalize() = 0;

private:

    // No copying or moving (see https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#cdefop-default-operations)
    MblDBusBinder(const MblDBusBinder&) = delete;
    MblDBusBinder & operator = (const MblDBusBinder&) = delete;
    MblDBusBinder(MblDBusBinder&&) = delete;
    MblDBusBinder& operator = (MblDBusBinder&&) = delete;    
};

} // namespace mbl

#endif // MblDBusBinder_h_