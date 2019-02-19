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
#include "cloud-connect-resource-broker/DBusAdapter.h"
#include "MblError.h"
#include "log.h"
#include "mbed-trace/mbed_trace.h"

#define TRACE_GROUP "ccrb-resource-broker-test"

////////////////////////////////////////////////////////////////////////////////////////////////////
class DBusAdapterTester  : public mbl::DBusAdapter {

public:

    DBusAdapterTester(mbl::ResourceBroker &ccrb);
    ~DBusAdapterTester();

    virtual mbl::MblError update_registration_status(
        const uintptr_t ipc_conn_handle, 
        const CloudConnectStatus reg_status) override;

};

DBusAdapterTester::DBusAdapterTester(mbl::ResourceBroker &ccrb)
: DBusAdapter(ccrb)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}
DBusAdapterTester::~DBusAdapterTester()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
}

mbl::MblError DBusAdapterTester::update_registration_status(
    const uintptr_t /*ipc_conn_handle*/, 
    const CloudConnectStatus /*reg_status*/)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    return mbl::Error::None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
class ResourceBrokerTester {

public:
    
    ResourceBrokerTester();
    ~ResourceBrokerTester();

    mbl::MblError init();

    void add_objects(const M2MObjectList& object_list);
    void register_update();

    mbl::MblError register_resources(
        const uintptr_t ipc_conn_handle, 
        const std::string &appl_resource_definition_json,
        CloudConnectStatus &out_status,
        std::string &out_access_token);

private:

    mbl::ResourceBroker cloud_connect_resource_broker_;
    DBusAdapterTester dbus_adapter_tester_;
};

ResourceBrokerTester::ResourceBrokerTester()
: dbus_adapter_tester_(cloud_connect_resource_broker_)
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

    // Set Resource Broker function pointers to point to this class instead of to Mbed Client
    // This replaces what cloud_connect_resource_broker_.init() usually does so dont call it...
    cloud_connect_resource_broker_.register_update_func_ = std::bind(&ResourceBrokerTester::register_update, this);
    cloud_connect_resource_broker_.add_objects_func_ = std::bind(static_cast<void(ResourceBrokerTester::*)(const M2MObjectList&)>(&ResourceBrokerTester::add_objects), this, std::placeholders::_1);

    cloud_connect_resource_broker_.ipc_ = std::make_unique<DBusAdapterTester>(cloud_connect_resource_broker_);

    return mbl::Error::None;
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

    mbl::MblError status = cloud_connect_resource_broker_.register_resources(
    ipc_conn_handle, 
    appl_resource_definition_json,
    out_status,
    out_access_token);

    if(mbl::Error::None != status) {
        tr_error("%s: register_resources failed with error: %s",
            __PRETTY_FUNCTION__,
            mbl::MblError_to_str(status));
        return status;
    }

    //Call Application Endpoint to notify registration was successful
    mbl::ResourceBroker::SPApplicationEndpoint app_endpoint = 
        cloud_connect_resource_broker_.app_endpoints_map_[out_access_token];

    // Make sure application is not yet registered
    if(app_endpoint->is_registered()) {
        tr_error("%s: Application (access_token: %s) should have not marked as registered.",
            __PRETTY_FUNCTION__,
            out_access_token.c_str());
        return mbl::Error::Unknown;
    }

    tr_debug("%s: Notify Application (access_token: %s) that registration was successful",
        __PRETTY_FUNCTION__,
        out_access_token.c_str());

    app_endpoint->handle_register_cb();

    if(!app_endpoint->is_registered()) {
        tr_error("%s: Application (access_token: %s) should have been marked as registered.",
            __PRETTY_FUNCTION__,
            out_access_token.c_str());
        return mbl::Error::Unknown;
    }

    return mbl::Error::None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

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
