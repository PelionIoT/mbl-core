/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "DBusAdapter.h"
#include "DBusAdapter_Impl.h"
#include "MblError.h"
#include "ResourceBroker.h"
#include "TestInfraAppThread.h"
#include "TestInfra.h"
#include "TestInfra_DBusAdapterTester.h"
#include "CloudConnectTrace.h"

#define TRACE_GROUP "dbus-gtest-infra"

using namespace mbl;


struct AdapterParameterizedData {
    DBusAdapter& adapater;
    size_t test_array_index;
};
 
class MessageReplyTest_ResourceBroker: public ResourceBroker {

private:
    MblError set_failure_parameters(
        const std::string & input,
        CloudConnectStatus & output_status)
    {
        if(input == "Set_Success_Return_Error")
        {
            output_status = STATUS_SUCCESS;
            return Error::DBA_IllegalState;
        }

        if(input == "Set_Error_Return_Error")
        {
            output_status = ERR_INTERNAL_ERROR;
            return Error::DBA_IllegalState;
        }

        if(input == "Set_Error_Return_Success")
        {
            output_status = ERR_FAILED;
            return Error::None;
        }

        return Error::None;
    }

public:
    MblError register_resources(
        const uintptr_t /*unused*/, 
        const std::string & json,
        CloudConnectStatus & status,
        std::string & token) override
    {
        TR_DEBUG("Enter");    
        if(json == "Set_Success_Return_Success")
        {
            token = std::string(json) + std::string("_token");
            status = STATUS_SUCCESS;
            return Error::None;
        }

        return set_failure_parameters(json, status);
    }

    MblError deregister_resources(
            const uintptr_t /*unused*/, 
            const std::string & token,
            CloudConnectStatus & status) override
    {
        TR_DEBUG("Enter");    
        if(token == "Set_Success_Return_Success")
        {
            status = STATUS_SUCCESS;
            return Error::None;
        }

        return set_failure_parameters(token, status);
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

const RegisterResources_entry RegisterResources_test_array[] = {

    // string input                
    {
        "Set_Success_Return_Success",        // input_json_data
        STATUS_SUCCESS, /* not relevant */   // expected_status
        "Set_Success_Return_Success_token",  // expected_access_token
        "" /* not relevant */                // expected_sd_bus_error_name
    },

    {
        "Set_Error_Return_Error",            // input_json_data
        FIRST_ERROR,    /* not relevant */   // expected_status
        "",       /* not relevant */         // expected_access_token             
        CLOUD_CONNECT_ERR_INTERNAL_ERROR     // expected_sd_bus_error_name
    },                             

    {
        "Set_Success_Return_Error",          // input_json_data
        FIRST_ERROR,    /* not relevant */   // expected_status
        "",      /* not relevant */          // expected_access_token
        CLOUD_CONNECT_ERR_INTERNAL_ERROR     // expected_sd_bus_error_name
    },

    {
        "Set_Error_Return_Success",          // input_json_data
        FIRST_ERROR,    /* not relevant */   // expected_status
        "",      /* not relevant */          // expected_access_token
        CLOUD_CONNECT_ERR_FAILED             // expected_sd_bus_error_name
    }
};

static int AppThreadCb_validate_adapter_register_resources(AppThread *app_thread, void *user_data)
{
    TR_DEBUG("Enter");
    assert(app_thread);
    assert(user_data);
    
    AdapterParameterizedData *adapter_param_data = static_cast<AdapterParameterizedData *>(user_data);
    const RegisterResources_entry &test_data = RegisterResources_test_array[adapter_param_data->test_array_index];

    sd_bus_message *reply = nullptr;
    sd_objects_cleaner<sd_bus_message> reply_cleaner (reply, sd_bus_message_unref);

    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_objects_cleaner<sd_bus_error> error_cleaner (&error, sd_bus_error_free);

    int test_result = TEST_SUCCESS;
    int r = sd_bus_call_method(
                    app_thread->get_connection_handle(),
                    DBUS_CLOUD_SERVICE_NAME,
                    DBUS_CLOUD_CONNECT_OBJECT_PATH,
                    DBUS_CLOUD_CONNECT_INTERFACE_NAME,
                    "RegisterResources",
                    &error,
                    &reply,
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
        uint32_t out_status = ERR_FAILED;
        const char* out_access_token = nullptr;
        r = sd_bus_message_read(reply, "us", &out_status, &out_access_token);
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

    DBusAdapter &adapter = adapter_param_data->adapater;
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
                        ::testing::Range(0, (int)ARRAY_SIZE(RegisterResources_test_array), 1));
TEST_P(ValidateRegisterResources, BasicMethodReply)
{
    GTEST_LOG_START_TEST;    
    MessageReplyTest_ResourceBroker ccrb;
    DBusAdapter adapter(ccrb);
    ASSERT_EQ(adapter.init(), MblError::None);        

    AdapterParameterizedData userdata = {adapter, (size_t)GetParam()};

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

struct DeregisterResources_entry{
    const char *input_token_data;
    CloudConnectStatus expected_status;
    const char *expected_sd_bus_error_name;
};

const DeregisterResources_entry DeregisterResources_test_array[] = {

    // string input                
    {
        "Set_Success_Return_Success",        // input_token_data
        STATUS_SUCCESS, /* not relevant */   // expected_status
        "" /* not relevant */                // expected_sd_bus_error_name
    },

    {
        "Set_Error_Return_Error",            // input_token_data
        FIRST_ERROR,    /* not relevant */   // expected_status
        CLOUD_CONNECT_ERR_INTERNAL_ERROR     // expected_sd_bus_error_name
    },                             

    {
        "Set_Success_Return_Error",          // input_token_data
        FIRST_ERROR,    /* not relevant */   // expected_status
        CLOUD_CONNECT_ERR_INTERNAL_ERROR     // expected_sd_bus_error_name
    },

    {
        "Set_Error_Return_Success",          // input_token_data
        FIRST_ERROR,    /* not relevant */   // expected_status
        CLOUD_CONNECT_ERR_FAILED             // expected_sd_bus_error_name
    }
};

static int AppThreadCb_validate_adapter_deregister_resources(AppThread *app_thread, void *user_data)
{
    TR_DEBUG("Enter");
    assert(app_thread);
    assert(user_data);
    
    AdapterParameterizedData *adapter_param_data = static_cast<AdapterParameterizedData *>(user_data);
    const DeregisterResources_entry &test_data = DeregisterResources_test_array[adapter_param_data->test_array_index];

    sd_bus_message *reply = nullptr;
    sd_objects_cleaner<sd_bus_message> reply_cleaner (reply, sd_bus_message_unref);

    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_objects_cleaner<sd_bus_error> error_cleaner (&error, sd_bus_error_free);

    int test_result = TEST_SUCCESS;
    int r = sd_bus_call_method(
                    app_thread->get_connection_handle(),
                    DBUS_CLOUD_SERVICE_NAME,
                    DBUS_CLOUD_CONNECT_OBJECT_PATH,
                    DBUS_CLOUD_CONNECT_INTERFACE_NAME,
                    "DeregisterResources",
                    &error,
                    &reply,
                    "s",
                    test_data.input_token_data);

    if (r < 0) {
        // message reply error gotten
        // compare to expected errors
        if(0 != strcmp(error.name, test_data.expected_sd_bus_error_name)) {
            TR_ERR("Actual error(%s) != Expected error(%s)", 
                error.name, test_data.expected_sd_bus_error_name);
            set_test_result(test_result, TEST_FAILED_EXPECTED_RESULT_MISMATCH);
        }
    } else {
        // method reply gotten
        // read status and access_token
        uint32_t out_status = ERR_FAILED;
        r = sd_bus_message_read(reply, "u", &out_status);
        if (r < 0) {
            TR_ERR("sd_bus_message_read failed(err=%d)", r);
            set_test_result(test_result, TEST_FAILED_SD_BUS_SYSTEM_CALL_FAILED);
        } else {
            // compare to expected 
            if(test_data.expected_status != out_status)
            {
                TR_ERR("Actual status(%d) != Expected status(%d)", 
                    out_status, test_data.expected_status);
                set_test_result(test_result, TEST_FAILED_EXPECTED_RESULT_MISMATCH);
            }
        }
    }

    DBusAdapter &adapter = adapter_param_data->adapater;
    MblError adapter_finish_status = adapter.stop(MblError::None);
    if(MblError::None != adapter_finish_status)
    {
        TR_ERR("adapter->stop failed(err=%s)", MblError_to_str(adapter_finish_status));
        set_test_result(test_result, TEST_FAILED_ADAPTER_METHOD_FAILED);
    }

    return test_result;
}

class ValidateDeregisterResources : public testing::TestWithParam<int>{};
INSTANTIATE_TEST_CASE_P(AdapterParameterizedTest,
                        ValidateDeregisterResources,
                        ::testing::Range(0, (int)ARRAY_SIZE(DeregisterResources_test_array), 1));
TEST_P(ValidateDeregisterResources, BasicMethodReply)
{
    GTEST_LOG_START_TEST;    
    MessageReplyTest_ResourceBroker ccrb;
    DBusAdapter adapter(ccrb);
    ASSERT_EQ(adapter.init(), MblError::None);        

    AdapterParameterizedData userdata = {adapter, (size_t)GetParam()};

    AppThread app_thread(AppThreadCb_validate_adapter_deregister_resources, &userdata);
    ASSERT_EQ(app_thread.create(), 0);

    MblError stop_status;
    ASSERT_EQ(adapter.run(stop_status), MblError::None);

    void *retval = nullptr;
    ASSERT_EQ(app_thread.join(&retval), 0);
    ASSERT_EQ((uintptr_t)retval, TEST_SUCCESS);
    ASSERT_EQ(adapter.deinit(), MblError::None);
}

