/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "DBusAdapter.h"
#include "DBusAdapter_Impl.h"
#include "MblError.h"
#include "ResourceBroker.h"
#include "TestInfraAppThread.h"
#include "TestInfra.h"
#include "TestInfra_DBusAdapterTester.h"
#include "CloudConnectTrace.h"
#include "MbedCloudClient.h"

#include <gtest/gtest.h>
#include <semaphore.h>

#define TRACE_GROUP "ccrb-dbus-gtest"

using namespace mbl;


struct AdapterParameterizedData {

    AdapterParameterizedData(DBusAdapter& adapater, size_t test_array_index)
    : adapater_(adapater), test_array_index_(test_array_index) {}

    DBusAdapter& adapater_;
    size_t test_array_index_;
};
 
class MessageReplyTest_ResourceBroker: public ResourceBroker {

public:
    std::pair<CloudConnectStatus, std::string> register_resources(
        const mbl::IpcConnection & /*unused*/, 
        const std::string & json) override
    {
        TR_DEBUG_ENTER;
        std::pair<CloudConnectStatus, std::string> ret_pair(CloudConnectStatus::STATUS_SUCCESS,
                                                        std::string());
        if(json == "Return_Success")
        {
            ret_pair.second = std::string(json) + std::string("_token");
            return ret_pair;
        }

        ret_pair.first = CloudConnectStatus::ERR_INVALID_APPLICATION_RESOURCES_DEFINITION;
        return ret_pair;
    }

    CloudConnectStatus deregister_resources(
            const mbl::IpcConnection & /*unused*/, 
            const std::string & token) override
    {
        TR_DEBUG_ENTER;    
        if(token == "Return_Success")
        {
            return STATUS_SUCCESS;
        }

        return CloudConnectStatus::ERR_INVALID_ACCESS_TOKEN;
    }
};

/////////////////////////////////////////////////////////////////////////////
// RegisterResources test
/////////////////////////////////////////////////////////////////////////////

struct RegisterResources_entry{
    const char *input_json_data;
    CloudConnectStatus expected_status;
    const char *expected_access_token;
    const char *expected_sd_bus_error_name;
};

const static std::vector<RegisterResources_entry> RegisterResources_test_array = {

    // string input                
    {
        "Return_Success",                    // input_json_data
        STATUS_SUCCESS, /* not relevant */   // expected_status
        "Return_Success_token",              // expected_access_token
        "" /* not relevant */                // expected_sd_bus_error_name
    },

    {
        "Return_Error",                      // input_json_data
        ERR_FIRST,      /* not relevant */   // expected_status
        "",       /* not relevant */         // expected_access_token             
        CLOUD_CONNECT_ERR_INVALID_APPLICATION_RESOURCES_DEFINITION     // expected_sd_bus_error_name
    },                             

};

static int AppThreadCb_validate_adapter_register_resources(AppThread *app_thread, void *user_data)
{
    TR_DEBUG_ENTER;
    assert(app_thread);
    assert(user_data);
    
    AdapterParameterizedData *adapter_param_data = static_cast<AdapterParameterizedData *>(user_data);
    const RegisterResources_entry &test_data = RegisterResources_test_array[adapter_param_data->test_array_index_];

    sd_bus_message *m_reply = nullptr;
    sd_bus_object_cleaner<sd_bus_message> reply_cleaner (m_reply, sd_bus_message_unref);

    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_object_cleaner<sd_bus_error> error_cleaner (&error, sd_bus_error_free);

    int test_result = TEST_SUCCESS;
    int r = sd_bus_call_method(
                    app_thread->get_connection_handle(),
                    DBUS_CLOUD_SERVICE_NAME,
                    DBUS_CLOUD_CONNECT_OBJECT_PATH,
                    DBUS_CLOUD_CONNECT_INTERFACE_NAME,
                    "RegisterResources",
                    &error,
                    &m_reply,
                    "s",
                    test_data.input_json_data);

    if (r < 0) {
        // message reply error gotten
        // compare to expected errors
        if(0 != strcmp(error.name, test_data.expected_sd_bus_error_name)) {
            TR_ERR("Actual error(%s) != Expected error(%s)", 
                error.name, test_data.expected_sd_bus_error_name);
            set_test_result(test_result, TEST_FAILED_EXPECTED_RESULT_MISMATCH);
        }
    }else {
        // method reply gotten
        // read status and access_token
        uint32_t out_status = ERR_INTERNAL_ERROR;
        const char* out_access_token = nullptr;
        r = sd_bus_message_read(m_reply, "us", &out_status, &out_access_token);
        if (r < 0) {
            TR_ERR("sd_bus_message_read failed(err=%d)", r);
            set_test_result(test_result, TEST_FAILED_SD_BUS_SYSTEM_CALL_FAILED);
        } else {
            // compare to expected 
            if(0 != strcmp(out_access_token, test_data.expected_access_token))
            {
                TR_ERR("Actual access_token(%s) != Expected access_token(%s)", 
                    out_access_token, test_data.expected_access_token);
                set_test_result(test_result, TEST_FAILED_EXPECTED_RESULT_MISMATCH);
            }
            
            if(test_data.expected_status != out_status)
            {
                TR_ERR("Actual status(%d) != Expected status(%d)", 
                    out_status, test_data.expected_status);
                set_test_result(test_result, TEST_FAILED_EXPECTED_RESULT_MISMATCH);
            }
        }
    }

    // we stop adapter event loop from this thread instead of having one more additional thread
    DBusAdapter &adapter = adapter_param_data->adapater_;
    MblError adapter_finish_status = adapter.stop(MblError::None);
    if(MblError::None != adapter_finish_status)
    {
        TR_ERR("adapter->stop failed(err=%s)", MblError_to_str(adapter_finish_status));
        set_test_result(test_result, TEST_FAILED_ADAPTER_METHOD_FAILED);
    }

    return test_result;
}

class ValidateRegisterResources : public testing::TestWithParam<int>{};
INSTANTIATE_TEST_CASE_P(AdapterParameterizedTest,
                        ValidateRegisterResources,
                        ::testing::Range(0, (int)RegisterResources_test_array.size(), 1));
TEST_P(ValidateRegisterResources, BasicMethodReply)
{
    GTEST_LOG_START_TEST;    
    MessageReplyTest_ResourceBroker ccrb;
    DBusAdapter adapter(ccrb);
    ASSERT_EQ(adapter.init(), MblError::None);        

    AdapterParameterizedData userdata(adapter, (size_t)GetParam());

    AppThread app_thread(AppThreadCb_validate_adapter_register_resources, &userdata);
    ASSERT_EQ(app_thread.create(), 0);

    MblError stop_status;
    ASSERT_EQ(adapter.run(stop_status), MblError::None);

    void *retval = nullptr;
    ASSERT_EQ(app_thread.join(&retval), 0);
    ASSERT_EQ((uintptr_t)retval, TEST_SUCCESS);
    ASSERT_EQ(adapter.deinit(), MblError::None);
}
/////////////////////////////////////////////////////////////////////////////
// DeregisterResources test
/////////////////////////////////////////////////////////////////////////////

// struct DeregisterResources_entry{
//     const char *input_token_data;
//     CloudConnectStatus expected_status;
//     const char *expected_sd_bus_error_name;
// };

// const static std::vector<DeregisterResources_entry> DeregisterResources_test_array = {

//     // string input                
//     {
//         "Return_Success",                    // input_token_data
//         STATUS_SUCCESS, /* not relevant */   // expected_status
//         "" /* not relevant */                // expected_sd_bus_error_name
//     },

//     {
//         "Return_Error",                      // input_token_data
//         ERR_FIRST,    /* not relevant */     // expected_status
//         CLOUD_CONNECT_ERR_INVALID_ACCESS_TOKEN     // expected_sd_bus_error_name
//     },                             

// };

// static int AppThreadCb_validate_adapter_deregister_resources(AppThread *app_thread, void *user_data)
// {
//     TR_DEBUG_ENTER;
//     assert(app_thread);
//     assert(user_data);
    
//     AdapterParameterizedData *adapter_param_data = static_cast<AdapterParameterizedData *>(user_data);
//     const DeregisterResources_entry &test_data = DeregisterResources_test_array[adapter_param_data->test_array_index_];

//     sd_bus_message *m_reply = nullptr;
//     sd_bus_object_cleaner<sd_bus_message> reply_cleaner (m_reply, sd_bus_message_unref);

//     sd_bus_error error = SD_BUS_ERROR_NULL;
//     sd_bus_object_cleaner<sd_bus_error> error_cleaner (&error, sd_bus_error_free);

//     int test_result = TEST_SUCCESS;
//     int r = sd_bus_call_method(
//                     app_thread->get_connection_handle(),
//                     DBUS_CLOUD_SERVICE_NAME,
//                     DBUS_CLOUD_CONNECT_OBJECT_PATH,
//                     DBUS_CLOUD_CONNECT_INTERFACE_NAME,
//                     "DeregisterResources",
//                     &error,
//                     &m_reply,
//                     "s",
//                     test_data.input_token_data);

//     if (r < 0) {
//         // message reply error gotten
//         // compare to expected errors
//         if(0 != strcmp(error.name, test_data.expected_sd_bus_error_name)) {
//             TR_ERR("Actual error(%s) != Expected error(%s)", 
//                 error.name, test_data.expected_sd_bus_error_name);
//             set_test_result(test_result, TEST_FAILED_EXPECTED_RESULT_MISMATCH);
//         }
//     } else {
//         // method reply gotten
//         // read status and access_token
//         uint32_t out_status = ERR_INTERNAL_ERROR;
//         r = sd_bus_message_read(m_reply, "u", &out_status);
//         if (r < 0) {
//             TR_ERR("sd_bus_message_read failed(err=%d)", r);
//             set_test_result(test_result, TEST_FAILED_SD_BUS_SYSTEM_CALL_FAILED);
//         } else {
//             // compare to expected 
//             if(test_data.expected_status != out_status)
//             {
//                 TR_ERR("Actual status(%d) != Expected status(%d)", 
//                     out_status, test_data.expected_status);
//                 set_test_result(test_result, TEST_FAILED_EXPECTED_RESULT_MISMATCH);
//             }
//         }
//     }

//     // we stop adapter event loop from this thread instead of having one more additional thread
//     DBusAdapter &adapter = adapter_param_data->adapater_;
//     MblError adapter_finish_status = adapter.stop(MblError::None);
//     if(MblError::None != adapter_finish_status)
//     {
//         TR_ERR("adapter->stop failed(err=%s)", MblError_to_str(adapter_finish_status));
//         set_test_result(test_result, TEST_FAILED_ADAPTER_METHOD_FAILED);
//     }

//     return test_result;
// }

// class ValidateDeregisterResources : public testing::TestWithParam<int>{};
// INSTANTIATE_TEST_CASE_P(AdapterParameterizedTest,
//                         ValidateDeregisterResources,
//                         ::testing::Range(0, (int)DeregisterResources_test_array.size(), 1));
// TEST_P(ValidateDeregisterResources, BasicMethodReply)
// {
//     GTEST_LOG_START_TEST;    
//     MessageReplyTest_ResourceBroker ccrb;
//     DBusAdapter adapter(ccrb);
//     ASSERT_EQ(adapter.init(), MblError::None);        

//     AdapterParameterizedData userdata(adapter, (size_t)GetParam());

//     AppThread app_thread(AppThreadCb_validate_adapter_deregister_resources, &userdata);
//     ASSERT_EQ(app_thread.create(), 0);

//     MblError stop_status;
//     ASSERT_EQ(adapter.run(stop_status), MblError::None);

//     void *retval = nullptr;
//     ASSERT_EQ(app_thread.join(&retval), 0);
//     ASSERT_EQ((uintptr_t)retval, TEST_SUCCESS);
//     ASSERT_EQ(adapter.deinit(), MblError::None);
// }


/////////////////////////////////////////////////////////////////////////////
// DBusAdapter Validate maximal allowed connections enforced
/////////////////////////////////////////////////////////////////////////////



class ResourceBrokerMock : public ResourceBroker
{
public:
    virtual std::pair<CloudConnectStatus, std::string> 
    register_resources(const IpcConnection & , 
                       const std::string &) override
    {
        TR_DEBUG_ENTER; 
        return std::make_pair(CloudConnectStatus::STATUS_SUCCESS, "");        
    }
};

static int AppThreadCb_validate_max_allowed_connections_enforced(
    AppThread *app_thread,
    void * userdata)
{
    sd_bus_message *m_reply = nullptr;
    sd_bus_object_cleaner<sd_bus_message> reply_cleaner (m_reply, sd_bus_message_unref);
    sd_bus_error bus_error = SD_BUS_ERROR_NULL;
    DBusAdapter *adapter = static_cast<DBusAdapter*>(userdata);

    // Call on already active connection handle. The RegisterResources callback to CCRB 
    // is overwritten above by ResourceBrokerMock::register_resources
    int r = sd_bus_call_method(
                    app_thread->get_connection_handle(),
                    DBUS_CLOUD_SERVICE_NAME,
                    DBUS_CLOUD_CONNECT_OBJECT_PATH,
                    DBUS_CLOUD_CONNECT_INTERFACE_NAME,
                    "RegisterResources",
                    &bus_error,
                    &m_reply,
                    "s",
                    "resources_definition_file_1");
    if (r < 0) {
        TR_ERR("sd_bus_call_method failed with r=%d (%s)", r, strerror(-r));
        adapter->stop(MblError::None);
        pthread_exit(reinterpret_cast<void *>(-r));
    }

    // Open new connection handle and send RegisterResources again
    // This time we expect it to fail with error ERR_NUM_ALLOWED_CONNECTIONS_EXCEEDED
    sd_bus_error_free(&bus_error);
    bus_error = SD_BUS_ERROR_NULL;
    sd_bus * second_connection_handle = nullptr;
    sd_bus_object_cleaner<sd_bus> connection_cleaner (second_connection_handle, sd_bus_unref);    
    r = sd_bus_open_user(&second_connection_handle);
    if (r < 0){
        TR_ERR("sd_bus_open_user failed with r=%d (%s)", r, strerror(-r));
        adapter->stop(MblError::None);
        pthread_exit(reinterpret_cast<void *>(-r));
    }
    if (nullptr == second_connection_handle){
        TR_ERR("sd_bus_open_user failed (second_connection_handle is NULL)");
        adapter->stop(MblError::None);
        pthread_exit(reinterpret_cast<void *>(-1000));
    }

    r = sd_bus_call_method(
                    second_connection_handle,
                    DBUS_CLOUD_SERVICE_NAME,
                    DBUS_CLOUD_CONNECT_OBJECT_PATH,
                    DBUS_CLOUD_CONNECT_INTERFACE_NAME,
                    "RegisterResources",
                    &bus_error,
                    &m_reply,
                    "s",
                    "resources_definition_file_2");
    // We expect a failure with specific error code (check error code and exact error name)
    bool b1 = r != CloudConnectStatus::ERR_NUM_ALLOWED_CONNECTIONS_EXCEEDED;
    bool b2 = !strcmp(CLOUD_CONNECT_ERR_NUM_ALLOWED_CONNECTIONS_EXCEEDED, bus_error.name);
    if (!b1 || !b2)        
    {
        TR_ERR("unexpected error code r=%d (%s) or invalid error name=%s",
            r , strerror(-r), bus_error.name);
        adapter->stop(MblError::None);
        pthread_exit(reinterpret_cast<void *>(-1001));
    }

    // we stop adapter event loop from this thread instead of having one more additional thread    
    MblError status = adapter->stop(MblError::None);
    if(MblError::None != status){
        TR_ERR("adapter->stop failed(err=%s)", MblError_to_str(status));        
         pthread_exit(reinterpret_cast<void *>(-1002));
    }

    sd_bus_unref(second_connection_handle);
    return 0;
}

TEST(DBusAdapterValidate2, max_allowed_connections_enforced)
{
    GTEST_LOG_START_TEST;    
    ResourceBrokerMock ccrb;
    DBusAdapter adapter(ccrb);
    ASSERT_EQ(adapter.init(), MblError::None);        

    // Start an application thread which simulates 2 connections.
    // First connection registers succesfully. Second connetion try to register and fails.
    AppThread app_thread(AppThreadCb_validate_max_allowed_connections_enforced, &adapter);
    ASSERT_EQ(app_thread.create(), 0);

    // run the adapter
    MblError stop_status;
    ASSERT_EQ(adapter.run(stop_status), MblError::None);

    //check that  actual test on client succeeded and deinit adapter
    void *retval = nullptr;
    ASSERT_EQ(app_thread.join(&retval), 0);
    ASSERT_EQ((uintptr_t)retval, MblError::None);
    ASSERT_EQ(adapter.deinit(), MblError::None);
}