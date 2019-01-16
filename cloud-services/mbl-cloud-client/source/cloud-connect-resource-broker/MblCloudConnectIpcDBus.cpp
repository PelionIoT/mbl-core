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

#include "MblCloudConnectIpcDBus.h"

#include <json/json.h>
#include <json/reader.h>
#include "mbed-trace/mbed_trace.h"

#define TRACE_GROUP "CCRB-IPCDBUS"

namespace mbl {

MblCloudConnectIpcDBus::MblCloudConnectIpcDBus()
{
    tr_info("MblCloudConnectIpcDBus::MblCloudConnectIpcDBus");
}

MblCloudConnectIpcDBus::~MblCloudConnectIpcDBus()
{
    tr_debug("MblCloudConnectIpcDBus::~MblCloudConnectIpcDBus");
}

MblError MblCloudConnectIpcDBus::Init()
{
    tr_info("MblCloudConnectIpcDBus::Init");

    //testing
    string text ="{ \"people\": [{\"id\": 1, \"name\":\"MIKE\",\"surname\":\"TAYLOR\"}, {\"id\": 2, \"name\":\"TOM\",\"surname\":\"JERRY\"} ]}";

tr_info("MblCloudConnectIpcDBus::Init 1");
    Json::CharReaderBuilder builder;
tr_info("MblCloudConnectIpcDBus::Init 2");    
    Json::CharReader * reader = builder.newCharReader();
tr_info("MblCloudConnectIpcDBus::Init 3");    

    Json::Value root;
    string errors;

tr_info("MblCloudConnectIpcDBus::Init 4");
    bool parsingSuccessful = reader->parse(text.c_str(), text.c_str() + text.size(), &root, &errors);
tr_info("MblCloudConnectIpcDBus::Init 5");    
    delete reader;

tr_info("MblCloudConnectIpcDBus::Init 6");    
    if ( !parsingSuccessful )
    {
        tr_info("MblCloudConnectIpcDBus::Init 7");    
        tr_error("Error parsing the string");
        return Error::None;
    }

tr_info("MblCloudConnectIpcDBus::Init 8");    
    for( Json::Value::iterator itr = root.begin() ; itr != root.end() ; itr++ )
    {
        tr_info("Name= %s", (*itr)["name"].asString().c_str());
    }
    
tr_info("MblCloudConnectIpcDBus::Init 9");        
    return Error::None;
}

} // namespace mbl
