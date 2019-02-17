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
#include "application_init.h"
#include "log.h"
#include "MblCloudClient.h"
//#include "signals.h"
//#include "MblError.h"
#include "mbed-trace/mbed_trace.h"

//#include <cerrno>
//#include <cstdio>
//#include <unistd.h>

#define TRACE_GROUP "ccrb-resource-broker-test"

////////////////////////////////////////////////////////////////////////////////

class ResourceBrokerTester {

public:
    
    mbl::MblError init();
    mbl::MblError terminate();

    mbl::MblError register_resources(
        const uintptr_t ipc_conn_handle, 
        const std::string &appl_resource_definition_json,
        CloudConnectStatus &out_status,
        std::string &out_access_token);
    
    mbl::MblError deregister_resources(
        const uintptr_t ipc_conn_handle, 
        const std::string &access_token,
        CloudConnectStatus &out_status);

private:
    mbl::MblCloudClient mbl_cloud_client_;        
};

mbl::MblError ResourceBrokerTester::init()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    mbl::application_init();


    mbl_cloud_client_.register_handlers();
    
    // Add empty ObjectList which will be used to register the device for the first time
    // in cloud_client_setup()
    mbl_cloud_client_.add_resources();

    const mbl::MblError ccs_err = mbl_cloud_client_.cloud_client_setup();
    if (ccs_err != mbl::Error::None) {
        tr_err("cloud_client_setup failed! (%s)", mbl::MblError_to_str(ccs_err));
        return ccs_err;
    }

    // start running CCRB module
    const mbl::MblError ccrb_start_err = mbl_cloud_client_.cloud_connect_resource_broker_.start(mbl_cloud_client_.cloud_client_);
    if(mbl::Error::None != ccrb_start_err) {
        tr_err("resource broker start() failed! (%s)", mbl::MblError_to_str(ccrb_start_err));
        return mbl::MblError::Unknown; //TODO: need new error?
    }

    return mbl::Error::None;

}

mbl::MblError ResourceBrokerTester::terminate()
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    return mbl_cloud_client_.cloud_connect_resource_broker_.stop();
}

mbl::MblError ResourceBrokerTester::register_resources(
        const uintptr_t ipc_conn_handle, 
        const std::string &appl_resource_definition_json,
        CloudConnectStatus &out_status,
        std::string &out_access_token)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    return mbl_cloud_client_.cloud_connect_resource_broker_.register_resources(
        ipc_conn_handle,
        appl_resource_definition_json,
        out_status,
        out_access_token);
}

mbl::MblError ResourceBrokerTester::deregister_resources(
    const uintptr_t ipc_conn_handle, 
    const std::string &access_token,
    CloudConnectStatus &out_status)
{
    tr_debug("%s", __PRETTY_FUNCTION__);
    return mbl_cloud_client_.cloud_connect_resource_broker_.deregister_resources(
        ipc_conn_handle,
        access_token,
        out_status);
}
////////////////////////////////////////////////////////////////////////////////


TEST(Resource_Broker_Positive, test1) {

    mbl::MblError status = mbl::Error::None;
    ResourceBrokerTester resource_broker_tester;

    status = resource_broker_tester.init();
    ASSERT_TRUE(mbl::Error::None != status);

    const uintptr_t ipc_conn_handle = 0;
    const std::string appl_resource_definition_json = R"({"77777" : { "11" : { "111" : { "mode" : "static", "resource_type" : "reset_button", "type" : "string", "value": "string_val", "operations" : ["get"], "multiple_instance" : false} } } })";
    CloudConnectStatus out_status;
    std::string out_access_token;

    status = resource_broker_tester.register_resources(
        ipc_conn_handle,
        appl_resource_definition_json,
        out_status,
        out_access_token);
    ASSERT_TRUE(mbl::Error::None != status);

    status = resource_broker_tester.terminate();
    ASSERT_TRUE(mbl::Error::None != status);
}
