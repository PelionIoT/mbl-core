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

    string text ="{ \"book\":\"Alice in Wonderland\", \"year\":1865, \"characters\": [{\"name\":\"Jabberwock\", \"chapter\":1}, {\"name\":\"Cheshire Cat\", \"chapter\":6}, {\"name\":\"Mad Hatter\", \"chapter\":7} ]}";

    Json::CharReaderBuilder builder;
    Json::CharReader * reader = builder.newCharReader();

    Json::Value obj;
    string errors;

    bool parsingSuccessful = reader->parse(text.c_str(), text.c_str() + text.size(), &obj, &errors);
    delete reader;

    if ( !parsingSuccessful )
    {
        tr_error("Error parsing the string");
        return Error::None;
    }

    tr_info( "Book: %s", obj["book"].asString().c_str() );
    tr_info( "Year: %d", obj["year"].asUInt() );
    const Json::Value& characters = obj["characters"]; // array of characters

    for (Json::Value::ArrayIndex i = 0; i < characters.size(); i++)
    {
        tr_info( "name: %s", characters[i]["name"].asString().c_str());
        tr_info( "chapter: %d", characters[i]["chapter"].asUInt());
    }

    tr_info("MblCloudConnectIpcDBus::Init 4");    
    return Error::None;
}

} // namespace mbl
