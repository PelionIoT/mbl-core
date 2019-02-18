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

#include <gtest/gtest.h>

#include "cloud-connect-resource-broker/ResourceBroker.h"
#include "MblError.h"
#include "log.h"
#include "mbed-trace/mbed_trace.h"

#define TRACE_GROUP "ccrb-resource-broker-test"

////////////////////////////////////////////////////////////////////////////////

class ResourceBrokerTester {

public:
    
    ResourceBrokerTester();
    ~ResourceBrokerTester();

    mbl::MblError init();
    void regsiter_callback_handlers();

    void add_objects(const M2MObjectList& object_list);
    void register_update();

    mbl::MblError register_resources(
        const uintptr_t ipc_conn_handle, 
        const std::string &appl_resource_definition_json,
        CloudConnectStatus &out_status,
        std::string &out_access_token);

private:

    mbl::ResourceBroker cloud_connect_resource_broker_;
};

ResourceBrokerTester::ResourceBrokerTester()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

ResourceBrokerTester::~ResourceBrokerTester()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

mbl::MblError ResourceBrokerTester::init()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    //return cloud_connect_resource_broker_.init();

    // Set function pointers to point to this class
    // This part should come after cloud_connect_resource_broker_.init()

    cloud_connect_resource_broker_.register_update_func_ = std::bind(&ResourceBrokerTester::register_update, this);
    cloud_connect_resource_broker_.add_objects_func_ = std::bind(static_cast<void(ResourceBrokerTester::*)(const M2MObjectList&)>(&ResourceBrokerTester::add_objects), this, std::placeholders::_1);
    cloud_connect_resource_broker_.regsiter_callback_handlers_func_ = std::bind(&ResourceBrokerTester::regsiter_callback_handlers, this);

    return mbl::Error::None;
}

void ResourceBrokerTester::regsiter_callback_handlers()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}
void ResourceBrokerTester::add_objects(const M2MObjectList& /*object_list*/)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    // if(!object_list.empty()) {
    //     M2MObjectList::const_iterator it;
    //     it = object_list.begin();
    //     for (; it!= object_list.end(); it++) {
    //         tr_debug("%s", __PRETTY_FUNCTION__);
    //     }
    // }
}

void ResourceBrokerTester::register_update()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

mbl::MblError ResourceBrokerTester::register_resources(
    const uintptr_t ipc_conn_handle, 
    const std::string &appl_resource_definition_json,
    CloudConnectStatus &out_status,
    std::string &out_access_token)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    return cloud_connect_resource_broker_.register_resources(
    ipc_conn_handle, 
    appl_resource_definition_json,
    out_status,
    out_access_token);
}


////////////////////////////////////////////////////////////////////////////////


TEST(Resource_Broker_Positive, test1) {

    tr_debug("%s", __PRETTY_FUNCTION__);

    mbl::MblError status = mbl::Error::None;
    
    const uintptr_t ipc_conn_handle = 0;
    const std::string appl_resource_definition_json = R"({"77777" : { "11" : { "111" : { "mode" : "static", "resource_type" : "reset_button", "type" : "string", "value": "string_val", "operations" : ["get"], "multiple_instance" : false} } } })";
    CloudConnectStatus out_status;
    std::string out_access_token;

    ResourceBrokerTester resource_broker_tester;

    tr_debug("%s: Call resource_broker_tester.init", __PRETTY_FUNCTION__);
    status = resource_broker_tester.init();
    ASSERT_TRUE(mbl::Error::None == status);

    tr_debug("%s: Call resource_broker_tester.register_resources", __PRETTY_FUNCTION__);
    status = resource_broker_tester.register_resources(
        ipc_conn_handle,
        appl_resource_definition_json,
        out_status,
        out_access_token);
    ASSERT_TRUE(mbl::Error::None == status);
}
