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


#ifndef MblCloudConnectIpcInterface_h_
#define MblCloudConnectIpcInterface_h_

namespace mbl {

/*! \file MblCloudConnectIpcInterface.h
 *  \brief MblCloudConnectIpcInterface.
 *  This class provides an interface for all IPC mechanisms
 */
class MblCloudConnectIpcInterface {

public:

    virtual ~MblCloudConnectIpcInterface(){};

    // Init API skeleton, later on the return value should be changed from int to somehting else
    virtual int Init() = 0;

    // Terminate API skeleton, later on the return value should be changed from int to somehting else
    virtual int Terminate() = 0;

protected:
    
    MblCloudConnectIpcInterface(){}

private:

    // Prevent instantiating / copy of this class 
    MblCloudConnectIpcInterface(const MblCloudConnectIpcInterface& other);
    MblCloudConnectIpcInterface& operator=(const MblCloudConnectIpcInterface& other);
};

} // namespace mbl

#endif // MblCloudConnectIpcInterface_h_
