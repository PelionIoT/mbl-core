/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "DBusAdapter.h"
#include "CloudConnectTrace.h"
#include "DBusAdapterCommon.h"
#include "DBusAdapter_Impl.h"
#include "DBusCloudConnectNames.h"
#include "MbedCloudClient.h"
#include "MblError.h"
#include "ResourceBrokerMockBase.h"
#include "TestInfra.h"
#include "TestInfraAppThread.h"
#include "TestInfra_DBusAdapterTester.h"

#include <gtest/gtest.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>//////////////////////////////////////////////////////////////////////////////////////////////
#define TRACE_GROUP "ccrb-dbus-gtest"

using namespace mbl;

/**
 * @brief This function send stop message to mailbox
 * 
 * @param adapter pointer
 */
static MblError send_adapter_stop_message(DBusAdapter * /*adapter*/)
{
/*    assert(adapter);
    MailboxMsg_Exit message_exit;
    message_exit.stop_status = MblError::None;
    MailboxMsg msg(message_exit, sizeof(message_exit));
    return adapter->send_mailbox_msg(msg);
*/
    kill(getpid(),SIGUSR1);
    return MblError::None;
}

// string value used in Set/Get resources values tests
static const std::string string_value = "string_value";
// resource path used in Set/Get resources values tests
static const char* resource_path = "/111/22/3";

struct AdapterParameterizedData
{

    AdapterParameterizedData(DBusAdapter& adapater, size_t test_array_index)
        : adapater_(adapater), test_array_index_(test_array_index)
    {
    }

    DBusAdapter& adapater_;
    size_t test_array_index_;
};
 
class MessageReplyTest_ResourceBroker: public ResourceBrokerMockBase {

public:
    MessageReplyTest_ResourceBroker(ResourceDataType data_type = STRING,
                                    const std::string& expected_path = "",
                                    const std::string& expected_val = "")
        : data_type_(data_type), expected_path_(expected_path)
    {
        switch (data_type)
        {
        case STRING: expected_string_val_ = expected_val; break;

        case INTEGER:
        {
            size_t size = 0;
            expected_int64_val_ = std::stoll(expected_val, &size);
            assert(size == expected_val.size());
            break;
        }

        default: assert(0 && "Bad test input format: unsupported resource type");
        }
    }

    std::pair<CloudConnectStatus, std::string> register_resources(mbl::IpcConnection /*unused*/,
                                                                  const std::string& json) override
    {
        TR_DEBUG_ENTER;
        std::pair<CloudConnectStatus, std::string> ret_pair(CloudConnectStatus::STATUS_SUCCESS,
                                                            std::string());
        if (json == "Return_Success") {
            ret_pair.second = std::string(json) + std::string("_token");
            return ret_pair;
        }

        ret_pair.first = CloudConnectStatus::ERR_INVALID_APPLICATION_RESOURCES_DEFINITION;
        return ret_pair;
    }

    CloudConnectStatus deregister_resources(mbl::IpcConnection /*unused*/,
                                            const std::string& token) override
    {
        TR_DEBUG_ENTER;
        if (token == "Return_Success") {
            return STATUS_SUCCESS;
        }

        return CloudConnectStatus::ERR_INVALID_ACCESS_TOKEN;
    }

    CloudConnectStatus set_resources_values(IpcConnection /*unused*/,
                                            const std::string& token,
                                            std::vector<ResourceSetOperation>& inout_set_operations)
    {
        // check expected resource path and value
        for (auto operation : inout_set_operations) {

            std::string path = operation.input_data_.get_path();
            if (path != std::string(expected_path_)) {

                TR_ERR(
                    "Actual path(%s) != Expected path(%s)", path.c_str(), expected_path_.c_str());
                return CloudConnectStatus::ERR_INVALID_RESOURCE_PATH;
            }

            ResourceDataType data_type = operation.input_data_.get_data_type();

            switch (data_type)
            {
            case STRING:
            {
                std::string value = operation.input_data_.get_value_string();

                if (value != expected_string_val_) {

                    TR_ERR("Actual value(%s) != Expected value(%s)",
                           value.c_str(),
                           expected_string_val_.c_str());
                    return CloudConnectStatus::ERR_INVALID_APPLICATION_RESOURCES_DEFINITION;
                }
                break;
            }
            case INTEGER:
            {
                int64_t value = operation.input_data_.get_value_integer();

                if (value != expected_int64_val_) {

                    TR_ERR("Actual value(%" PRIx64 ") != Expected value(%" PRIx64 ")",
                           value,
                           expected_int64_val_);
                    return CloudConnectStatus::ERR_INVALID_APPLICATION_RESOURCES_DEFINITION;
                }

                break;
            }

            default:
            {
                std::stringstream msg("Data type " + std::string(stringify(data_type)) +
                                      " is not supported");
                return CloudConnectStatus::ERR_INVALID_RESOURCE_TYPE;
            }
            }
        }

        CloudConnectStatus resource_ret_status = STATUS_SUCCESS;

        if (token == "Return_invalid_access_token") {
            return CloudConnectStatus::ERR_INVALID_ACCESS_TOKEN;
        }
        else if (token == "Return_invalid_resource_path")
        {
            resource_ret_status = CloudConnectStatus::ERR_INVALID_RESOURCE_PATH;
        }
        else if (token != "Return_Success")
        {
            return CloudConnectStatus::ERR_INTERNAL_ERROR;
        }

        for (auto& i : inout_set_operations) {
            i.output_status_ = resource_ret_status;
        }

        return STATUS_SUCCESS;
    }

    CloudConnectStatus get_resources_values(IpcConnection /*unused*/,
                                            const std::string& token,
                                            std::vector<ResourceGetOperation>& inout_get_operations)
    {
        TR_DEBUG_ENTER;

        CloudConnectStatus ret_status = STATUS_SUCCESS;
        CloudConnectStatus resource_ret_status = STATUS_SUCCESS;

        if (token == "Return_invalid_access_token") {
            return CloudConnectStatus::ERR_INVALID_ACCESS_TOKEN;
        }
        else if (token == "Return_invalid_resource_path")
        {
            resource_ret_status = CloudConnectStatus::ERR_INVALID_RESOURCE_PATH;
        }
        else if (token != "Return_Success")
        {
            return CloudConnectStatus::ERR_INTERNAL_ERROR;
        }

        for (auto& i : inout_get_operations) {

            ResourceDataType type = i.inout_data_.get_data_type();

            if (type == ResourceDataType::STRING) {
                i.inout_data_.set_value(expected_string_val_);
            }
            else if (type == ResourceDataType::INTEGER)
            {
                i.inout_data_.set_value(expected_int64_val_);
            }
            else
            { // invalid resource type - return
                i.output_status_ = CloudConnectStatus::ERR_INVALID_RESOURCE_TYPE;
                return ret_status;
            }

            i.output_status_ = resource_ret_status;
        }

        return STATUS_SUCCESS;
    }

private:
    ResourceDataType data_type_;
    // expected resource value
    std::string expected_string_val_;
    int64_t expected_int64_val_;
    // expected resource path
    const std::string expected_path_;
};

/////////////////////////////////////////////////////////////////////////////
// RegisterResources test
/////////////////////////////////////////////////////////////////////////////

struct RegisterResources_entry
{
    const char* input_json_data;
    const char* expected_access_token;
    const char* expected_sd_bus_error_name;
};

const static std::vector<RegisterResources_entry> RegisterResources_test_array = {

    // string input
    {
        "Return_Success",       // input_json_data
        "Return_Success_token", // expected_access_token
        "" /* not relevant */   // expected_sd_bus_error_name
    },

    {
        "Return_Error", // input_json_data
        "",
        /* not relevant */                                         // expected_access_token
        CLOUD_CONNECT_ERR_INVALID_APPLICATION_RESOURCES_DEFINITION // expected_sd_bus_error_name
    },
};

static int AppThreadCb_validate_adapter_register_resources(AppThread* app_thread, void* user_data)
{
    TR_DEBUG_ENTER;
    assert(app_thread);
    assert(user_data);

    AdapterParameterizedData* adapter_param_data =
        static_cast<AdapterParameterizedData*>(user_data);
    const RegisterResources_entry& test_data =
        RegisterResources_test_array[adapter_param_data->test_array_index_];

    sd_bus_message* m_reply = nullptr;
    sd_bus_object_cleaner<sd_bus_message> reply_cleaner(m_reply, sd_bus_message_unref);

    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_object_cleaner<sd_bus_error> error_cleaner(&error, sd_bus_error_free);

    int test_result = TEST_SUCCESS;
    int r = sd_bus_call_method(app_thread->get_connection_handle(),
                               DBUS_CLOUD_SERVICE_NAME,
                               DBUS_CLOUD_CONNECT_OBJECT_PATH,
                               DBUS_CLOUD_CONNECT_INTERFACE_NAME,
                               DBUS_CC_REGISTER_RESOURCES_METHOD_NAME,
                               &error,
                               &m_reply,
                               "s",
                               test_data.input_json_data);

    if (r < 0) {
        // message reply error gotten
        // compare to expected errors
        if (0 != strcmp(error.name, test_data.expected_sd_bus_error_name)) {
            TR_ERR("Actual error(%s) != Expected error(%s)",
                   error.name,
                   test_data.expected_sd_bus_error_name);
            set_test_result(test_result, TEST_FAILED_EXPECTED_RESULT_MISMATCH);
        }
    }
    else
    {
        // method reply gotten
        // read access_token
        const char* out_access_token = nullptr;
        r = sd_bus_message_read(m_reply, "s", &out_access_token);
        if (r < 0) {
            TR_ERR("sd_bus_message_read failed(err=%d)", r);
            set_test_result(test_result, TEST_FAILED_SD_BUS_SYSTEM_CALL_FAILED);
        }
        else
        {
            // compare to expected
            if (0 != strcmp(out_access_token, test_data.expected_access_token)) {
                TR_ERR("Actual access_token(%s) != Expected access_token(%s)",
                       out_access_token,
                       test_data.expected_access_token);
                set_test_result(test_result, TEST_FAILED_EXPECTED_RESULT_MISMATCH);
            }
        }
    }

    // we stop adapter event loop from this thread instead of having one more additional thread
    DBusAdapter& adapter = adapter_param_data->adapater_;
    MblError adapter_finish_status = send_adapter_stop_message(&adapter);
    if (MblError::None != adapter_finish_status) {
        TR_ERR("send_adapter_stop_message(err=%s)", MblError_to_str(adapter_finish_status));
        set_test_result(test_result, TEST_FAILED_ADAPTER_METHOD_FAILED);
    }

    return test_result;
}

class ValidateRegisterResources : public testing::TestWithParam<int>
{
};
INSTANTIATE_TEST_CASE_P(AdapterParameterizedTest,
                        ValidateRegisterResources,
                        ::testing::Range(0, (int) RegisterResources_test_array.size(), 1));
TEST_P(ValidateRegisterResources, BasicMethodReply)
{
    GTEST_LOG_START_TEST;
    MessageReplyTest_ResourceBroker ccrb_mock;
    DBusAdapter adapter(ccrb_mock);
    ccrb_mock.set_ipc_adapter(&adapter);
    ASSERT_EQ(adapter.init(), MblError::None);

    AdapterParameterizedData userdata(adapter, (size_t) GetParam());

    AppThread app_thread(AppThreadCb_validate_adapter_register_resources, &userdata);
    ASSERT_EQ(app_thread.create(), 0);

    MblError stop_status;
    ASSERT_EQ(adapter.run(stop_status), MblError::None);

    void* retval = nullptr;
    ASSERT_EQ(app_thread.join(&retval), 0);
    ASSERT_EQ((uintptr_t) retval, TEST_SUCCESS);
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

// static int AppThreadCb_validate_adapter_deregister_resources(AppThread *app_thread, void
// *user_data)
// {
//     TR_DEBUG_ENTER;
//     assert(app_thread);
//     assert(user_data);

//     AdapterParameterizedData *adapter_param_data = static_cast<AdapterParameterizedData
//     *>(user_data);
//     const DeregisterResources_entry &test_data =
//     DeregisterResources_test_array[adapter_param_data->test_array_index_];

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
//     MblError adapter_finish_status = send_adapter_stop_message(&adapter);
//     if(MblError::None != adapter_finish_status)
//     {
//         TR_ERR("send_adapter_stop_message(err=%s)", MblError_to_str(adapter_finish_status));
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

static int AppThreadCb_validate_max_allowed_connections_enforced(AppThread* app_thread,
                                                                 void* userdata)
{
    sd_bus_message* m_reply = nullptr;
    sd_bus_object_cleaner<sd_bus_message> reply_cleaner(m_reply, sd_bus_message_unref);
    sd_bus_error bus_error = SD_BUS_ERROR_NULL;
    DBusAdapter* adapter = static_cast<DBusAdapter*>(userdata);

    // Call on already active connection handle. The RegisterResources callback to CCRB
    // is overwritten above by ResourceBrokerMock1::register_resources
    int r = sd_bus_call_method(app_thread->get_connection_handle(),
                               DBUS_CLOUD_SERVICE_NAME,
                               DBUS_CLOUD_CONNECT_OBJECT_PATH,
                               DBUS_CLOUD_CONNECT_INTERFACE_NAME,
                               DBUS_CC_REGISTER_RESOURCES_METHOD_NAME,
                               &bus_error,
                               &m_reply,
                               "s",
                               "resources_definition_file_1");
    if (r < 0) {
        TR_ERR("sd_bus_call_method failed with r=%d (%s)", r, strerror(-r));
        send_adapter_stop_message(adapter);
        pthread_exit(reinterpret_cast<void*>(-r));
    }

    // Open new connection handle and send RegisterResources again
    // This time we expect it to fail with error ERR_NUM_ALLOWED_CONNECTIONS_EXCEEDED
    sd_bus_error_free(&bus_error);
    bus_error = SD_BUS_ERROR_NULL;
    sd_bus* second_connection_handle = nullptr;
    sd_bus_object_cleaner<sd_bus> connection_cleaner(second_connection_handle, sd_bus_unref);
    r = sd_bus_open_user(&second_connection_handle);
    if (r < 0) {
        TR_ERR("sd_bus_open_user failed with r=%d (%s)", r, strerror(-r));
        send_adapter_stop_message(adapter);
        pthread_exit(reinterpret_cast<void*>(-r));
    }
    if (nullptr == second_connection_handle) {
        TR_ERR("sd_bus_open_user failed (second_connection_handle is NULL)");
        send_adapter_stop_message(adapter);
        pthread_exit(reinterpret_cast<void*>(-1000));
    }

    r = sd_bus_call_method(second_connection_handle,
                           DBUS_CLOUD_SERVICE_NAME,
                           DBUS_CLOUD_CONNECT_OBJECT_PATH,
                           DBUS_CLOUD_CONNECT_INTERFACE_NAME,
                           DBUS_CC_REGISTER_RESOURCES_METHOD_NAME,
                           &bus_error,
                           &m_reply,
                           "s",
                           "resources_definition_file_2");
    // We expect a failure with specific error code (check error code and exact error name)
    bool b1 = r != CloudConnectStatus::ERR_NUM_ALLOWED_CONNECTIONS_EXCEEDED;
    bool b2 = !strcmp(CLOUD_CONNECT_ERR_NUM_ALLOWED_CONNECTIONS_EXCEEDED, bus_error.name);
    if (!b1 || !b2) {
        TR_ERR("unexpected error code r=%d (%s) or invalid error name=%s",
               r,
               strerror(-r),
               bus_error.name);
        send_adapter_stop_message(adapter);
        pthread_exit(reinterpret_cast<void*>(-1001));
    }

    // we stop adapter event loop from this thread instead of having one more additional thread
    MblError status = send_adapter_stop_message(adapter);
    if (MblError::None != status) {
        TR_ERR("send_adapter_stop_message(err=%s)", MblError_to_str(status));
        pthread_exit(reinterpret_cast<void*>(-1002));
    }

    sd_bus_unref(second_connection_handle);
    return 0;
}

TEST(DBusAdapter, enforce_single_connection_single_app_2_connections)
{
    GTEST_LOG_START_TEST;
    ResourceBrokerMockBase ccrb_mock;
    DBusAdapter adapter(ccrb_mock);
    ccrb_mock.set_ipc_adapter(&adapter);

    ASSERT_EQ(adapter.init(), MblError::None);

    // Start an application thread which simulates 2 connections.
    // First connection registers succesfully. Second connetion try to register and fails.
    AppThread app_thread(AppThreadCb_validate_max_allowed_connections_enforced, &adapter);
    ASSERT_EQ(app_thread.create(), 0);

    // run the adapter
    MblError stop_status;
    ASSERT_EQ(adapter.run(stop_status), MblError::None);

    // check that  actual test on client succeeded and deinit adapter
    void* retval = nullptr;
    ASSERT_EQ(app_thread.join(&retval), 0);
    ASSERT_EQ((uintptr_t) retval, MblError::None);
    ASSERT_EQ(adapter.deinit(), MblError::None);
}

/////////////////////////////////////////////////////////////////////////////
// DBusAdapter validate disconnection notification sent to CCRB
/////////////////////////////////////////////////////////////////////////////

/**
 * @brief Fixture class  which holds a callback and 2 statics vars used by the test
 *
 */
class DBusAdapaterFixture : public ::testing::Test
{
public:
    void SetUp() override
    {
        TR_DEBUG_ENTER;
        ASSERT_EQ(sem_init(&semaphore_, 0, 0), 0);
    }

    void TearDown() override
    {
        TR_DEBUG_ENTER;
        ASSERT_EQ(sem_destroy(&semaphore_), 0);
    }

    static int AppThreadCb_validate_client_disconnection_notification(AppThread* app_thread,
                                                                      void* userdata);

    // The active connection id, used in order to save traditional IPC mechanisms. The time of set
    // is always distinct from the time of get.
    static std::string active_connection_id_;

    // test result - true for success, false for failure.
    static TestResult test_result_;

    // a semaphore to synchronize between 2 threads - after disconnecting, the client thread
    // will wait till semaphore is raised
    static sem_t semaphore_;
};
std::string DBusAdapaterFixture::active_connection_id_ = "";
TestResult DBusAdapaterFixture::test_result_ = TEST_FAILED;
sem_t DBusAdapaterFixture::semaphore_;

// Overrides CCRB notify_connection_closed and register_resources
class ResourceBrokerMock2 : public ResourceBrokerMockBase
{
public:
    // Chckes that the reported closed source connection is equal to the actual unique connection id
    // as set by the application created thread before  calling RegisterResources
    void notify_connection_closed(IpcConnection source) override
    {
        TR_DEBUG_ENTER;

        if (source.get_connection_id() == DBusAdapaterFixture::active_connection_id_) {
            // test succuss! set true
            DBusAdapaterFixture::test_result_ = TEST_SUCCESS;
        }
        else
        {
            // if test fail, test_result_ is already false. just log the failure.
            TR_ERR("source connection id=%s is not equal to active_connection_id_=%s",
                   source.get_connection_id().c_str(),
                   DBusAdapaterFixture::active_connection_id_.c_str());
        }

        // post on semaphore
        int r = sem_post(&DBusAdapaterFixture::semaphore_);
        if (r != 0) {
            TR_ERR("sem_post failed with r=%d (%s)", errno, strerror(errno));
        }
    }

    virtual std::pair<CloudConnectStatus, std::string>
    register_resources(IpcConnection, const std::string&) override
    {
        TR_DEBUG_ENTER;
        // dummy success
        return std::make_pair(CloudConnectStatus::STATUS_SUCCESS, "token");
    }
};

// This callback is the AppThreat callback:
// 1) Sets the active connection id to fixture (global)
// 2) Calls RegisterResources
// 3) disconnect from bus
// 4) wait for service to signal on semaphore (this is a test feature) that disconnection has been
// notified. Service check on mocked callback that expected (set) active connection is the one
// which is being notified for disconnection
// 5) send stop to service to finish test with the test result.
int DBusAdapaterFixture::AppThreadCb_validate_client_disconnection_notification(
    AppThread* app_thread, void* userdata)
{
    sd_bus_message* m_reply = nullptr;
    sd_bus_object_cleaner<sd_bus_message> reply_cleaner(m_reply, sd_bus_message_unref);
    sd_bus_error bus_error = SD_BUS_ERROR_NULL;
    DBusAdapter* adapter = static_cast<DBusAdapter*>(userdata);

    // Before sending any message, set our unique connection ID.
    // This is validated in CCRB mocked callback notify_connection_closed
    active_connection_id_.assign(app_thread->get_unique_connection_id());
    assert(active_connection_id_.length() > 1);

    // Call RegisterResources. The register_resources call to CCRB
    // is overwritten above by ResourceBrokerMock2::register_resources
    int r = sd_bus_call_method(app_thread->get_connection_handle(),
                               DBUS_CLOUD_SERVICE_NAME,
                               DBUS_CLOUD_CONNECT_OBJECT_PATH,
                               DBUS_CLOUD_CONNECT_INTERFACE_NAME,
                               DBUS_CC_REGISTER_RESOURCES_METHOD_NAME,
                               &bus_error,
                               &m_reply,
                               "s",
                               "resources_definition_file_1");
    if (r < 0) {
        TR_ERR("sd_bus_call_method failed with r=%d (%s)", r, strerror(-r));
        send_adapter_stop_message(adapter);
        pthread_exit(reinterpret_cast<void*>(-r));
    }

    // disconnect. This should invoke ccrb callback notify_connection_closed on server
    app_thread->disconnect();

    // we must wait here to allow adapter to process the disconnection
    // this is a test, use sem_wait and not sem_timedwait to simplify code
    r = sem_wait(&DBusAdapaterFixture::semaphore_);
    if (r != 0) {
        TR_ERR("sd_bus_call_method failed with r=%d (%s)", errno, strerror(errno));
        send_adapter_stop_message(adapter);
        pthread_exit(reinterpret_cast<void*>(-errno));
    }

    // we stop adapter event loop from this thread instead of having one more additional thread
    MblError status = send_adapter_stop_message(adapter);
    if (MblError::None != status) {
        TR_ERR("send_adapter_stop_message(err=%s)", MblError_to_str(status));
        pthread_exit(reinterpret_cast<void*>(-1002));
    }

    return 0;
}

TEST(DBusAdapter, validate_client_disconnection_notification)
{
    GTEST_LOG_START_TEST;
    ResourceBrokerMock2 ccrb_mock;
    DBusAdapter adapter(ccrb_mock);
    ccrb_mock.set_ipc_adapter(&adapter);

    ASSERT_EQ(adapter.init(), MblError::None);

    // Start an application thread which will register to service, then close the connection.
    // the CCRB API notify_connection_closed is overriden. It validates that a disconnect
    // notification is sent by DBA.
    AppThread app_thread(
        DBusAdapaterFixture::AppThreadCb_validate_client_disconnection_notification, &adapter);
    ASSERT_EQ(app_thread.create(), 0);

    // run the adapter
    MblError stop_status;
    ASSERT_EQ(adapter.run(stop_status), MblError::None);

    // check that  actual test on client succeeded and deinit adapter
    void* retval = nullptr;
    ASSERT_EQ(app_thread.join(&retval), 0);
    ASSERT_EQ((uintptr_t) retval, MblError::None);
    ASSERT_EQ(adapter.deinit(), MblError::None);

    // check final test result
    ASSERT_EQ(DBusAdapaterFixture::test_result_, TEST_SUCCESS);
}
/////////////////////////////////////////////////////////////////////////////
// SetResources test
/////////////////////////////////////////////////////////////////////////////

struct SetResourcesValues_entry
{
    const char* input_access_token;
    const char* input_resource_path;
    ResourceDataType input_format;
    const char* input_resource_value;
    const char* expected_sd_bus_error_name;
};

const static std::vector<SetResourcesValues_entry> SetResourcesValues_test_array = {

    // string input, return success
    {
        "Return_Success",     // input_access_token
        resource_path,        // input_resource_path
        STRING,               // input_format
        string_value.c_str(), // input_resource_value
        "" /* not relevant */ // expected_sd_bus_error_name
    },
    // string input, return error
    {
        "Return_invalid_access_token",         // input_access_token
        resource_path,                         // input_resource_path
        STRING,                                // input_format
        string_value.c_str(),                  // input_resource_value
        CLOUD_CONNECT_ERR_INVALID_ACCESS_TOKEN // expected_sd_bus_error_name
    },
    // int64 input, return success
    {
        "Return_Success",     // input_access_token
        resource_path,        // input_resource_path
        INTEGER,              // input_format
        "100",                // input_resource_value
        "" /* not relevant */ // expected_sd_bus_error_name
    },
    // int64 input, return error
    {
        "Return_invalid_resource_path",         // input_access_token
        resource_path,                          // input_resource_path
        INTEGER,                                // input_format
        "100",                                  // input_resource_value
        CLOUD_CONNECT_ERR_INVALID_RESOURCE_PATH // expected_sd_bus_error_name
    },
    // very long path, string input, return success
    {
        "Return_Success",              // input_access_token
        "/55555555/44444444/33333333", // input_resource_path
        STRING,                        // input_format
        string_value.c_str(),          // input_resource_value
        "" /* not relevant */          // expected_sd_bus_error_name
    },
    // very long string input, return success
    {
        "Return_Success",                                                    // input_access_token
        resource_path,                                                       // input_resource_path
        STRING,                                                              // input_format
        "very_long_string_input_parameter_very_long_string_input_parameter", // input_resource_value
        "" /* not relevant */ // expected_sd_bus_error_name
    },
    // int64 maximal value input, return success
    {
        "Return_Success",      // input_access_token
        resource_path,         // input_resource_path
        INTEGER,               // input_format
        "9223372036854775807", // input_resource_value
        "" /* not relevant */  // expected_sd_bus_error_name
    },
    // int64 minimal value input, return success
    {
        "Return_Success",       // input_access_token
        resource_path,          // input_resource_path
        INTEGER,                // input_format
        "-9223372036854775807", // input_resource_value
        "" /* not relevant */   // expected_sd_bus_error_name
    },
};

static int AppThreadCb_validate_adapter_set_resources_values(AppThread* app_thread, void* user_data)
{
    TR_DEBUG_ENTER;
    assert(app_thread);
    assert(user_data);

    AdapterParameterizedData* adapter_param_data =
        static_cast<AdapterParameterizedData*>(user_data);
    const SetResourcesValues_entry& test_data =
        SetResourcesValues_test_array[adapter_param_data->test_array_index_];

    sd_bus_message* m_reply = nullptr;
    sd_bus_object_cleaner<sd_bus_message> reply_cleaner(m_reply, sd_bus_message_unref);

    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_object_cleaner<sd_bus_error> error_cleaner(&error, sd_bus_error_free);

    int test_result = TEST_SUCCESS;
    int r = 0;

    switch (test_data.input_format)
    {
    case STRING:

        r = sd_bus_call_method(app_thread->get_connection_handle(),
                               DBUS_CLOUD_SERVICE_NAME,
                               DBUS_CLOUD_CONNECT_OBJECT_PATH,
                               DBUS_CLOUD_CONNECT_INTERFACE_NAME,
                               DBUS_CC_SET_RESOURCES_VALUES_METHOD_NAME,
                               &error,
                               &m_reply,
                               "sa(sv)",
                               test_data.input_access_token,
                               1, /* number of array entries */
                               test_data.input_resource_path,
                               "s", /* Property variant type */
                               test_data.input_resource_value);
        break;

    case INTEGER:

        r = sd_bus_call_method(app_thread->get_connection_handle(),
                               DBUS_CLOUD_SERVICE_NAME,
                               DBUS_CLOUD_CONNECT_OBJECT_PATH,
                               DBUS_CLOUD_CONNECT_INTERFACE_NAME,
                               DBUS_CC_SET_RESOURCES_VALUES_METHOD_NAME,
                               &error,
                               &m_reply,
                               "sa(sv)",
                               test_data.input_access_token,
                               1, /* number of array entries */
                               test_data.input_resource_path,
                               "x", /* Property variant type */
                               std::stoll(test_data.input_resource_value));
        break;

    default:
        TR_ERR("Bad test input format: unsupported resource type %d", test_data.input_format);
        set_test_result(test_result, TEST_FAILED_INVALID_TEST_PARAMETERS);
        return test_result;
    }

    if (r < 0) {
        // message reply error gotten, compare to expected errors
        if (0 != strcmp(error.name, test_data.expected_sd_bus_error_name)) {

            TR_ERR("Actual error(%s) != Expected error(%s)",
                   error.name,
                   test_data.expected_sd_bus_error_name);
            set_test_result(test_result, TEST_FAILED_EXPECTED_RESULT_MISMATCH);
        }
        // check the error message
        if (std::string(test_data.input_access_token) == "Return_invalid_resource_path") {

            std::string expected_message("Set LWM2M resources failed: " +
                                         std::string(test_data.input_resource_path) + " : " +
                                         CloudConnectStatus_to_str(ERR_INVALID_RESOURCE_PATH));

            if (error.message != expected_message) {
                TR_ERR("Actual error message(%s) != Expected error message(%s)",
                       error.message,
                       expected_message.c_str());
                set_test_result(test_result, TEST_FAILED_EXPECTED_RESULT_MISMATCH);
            }
        }
    }
    else
    {
        // an empty method reply gotten
        r = sd_bus_message_read(m_reply, "");
        if (r < 0) {

            TR_ERR("sd_bus_message_read failed(err=%d)", r);
            set_test_result(test_result, TEST_FAILED_SD_BUS_SYSTEM_CALL_FAILED);
        }
    }

    // we stop adapter event loop from this thread instead of having one more additional thread
    DBusAdapter& adapter = adapter_param_data->adapater_;
    MblError adapter_finish_status = send_adapter_stop_message(&adapter);
    if (MblError::None != adapter_finish_status) {
        TR_ERR("send_adapter_stop_message(err=%s)", MblError_to_str(adapter_finish_status));
        set_test_result(test_result, TEST_FAILED_ADAPTER_METHOD_FAILED);
    }

    return test_result;
}

class ValidateSetResourcesValues : public testing::TestWithParam<int>
{
};
INSTANTIATE_TEST_CASE_P(AdapterParameterizedTest,
                        ValidateSetResourcesValues,
                        ::testing::Range(0, (int) SetResourcesValues_test_array.size(), 1));
TEST_P(ValidateSetResourcesValues, BasicMethodReply)
{
    GTEST_LOG_START_TEST;

    const SetResourcesValues_entry& test_data = SetResourcesValues_test_array[(size_t) GetParam()];
    MessageReplyTest_ResourceBroker ccrb_mock(
        test_data.input_format, test_data.input_resource_path, test_data.input_resource_value);
    DBusAdapter adapter(ccrb_mock);
    ccrb_mock.set_ipc_adapter(&adapter);    
    ASSERT_EQ(adapter.init(), MblError::None);

    AdapterParameterizedData userdata(adapter, (size_t) GetParam());

    AppThread app_thread(AppThreadCb_validate_adapter_set_resources_values, &userdata);
    ASSERT_EQ(app_thread.create(), 0);

    MblError stop_status;
    ASSERT_EQ(adapter.run(stop_status), MblError::None);

    void* retval = nullptr;
    ASSERT_EQ(app_thread.join(&retval), 0);
    ASSERT_EQ((uintptr_t) retval, TEST_SUCCESS);
    ASSERT_EQ(adapter.deinit(), MblError::None);
}

/////////////////////////////////////////////////////////////////////////////
// GetResources test
/////////////////////////////////////////////////////////////////////////////

struct GetResourcesValues_entry
{
    const char* input_access_token;
    const char* input_resource_path;
    ResourceDataType type;
    const char* input_resource_value;
    const char* expected_sd_bus_error_name;
};

const static std::vector<GetResourcesValues_entry> GetResourcesValues_test_array = {

    // string input, return success
    {
        "Return_Success",     // input_access_token
        resource_path,        // input_resource_path
        STRING,               // input_format
        string_value.c_str(), // input_resource_value
        "" /* not relevant */ // expected_sd_bus_error_name
    },
    // string input, return error
    {
        "Return_invalid_access_token",         // input_access_token
        resource_path,                         // input_resource_path
        STRING,                                // input_format
        string_value.c_str(),                  // input_resource_value
        CLOUD_CONNECT_ERR_INVALID_ACCESS_TOKEN // expected_sd_bus_error_name
    },
    // uint32 input, return success
    {
        "Return_Success",     // input_access_token
        resource_path,        // input_resource_path
        INTEGER,              // input_format
        "100",                // input_resource_value
        "" /* not relevant */ // expected_sd_bus_error_name
    },
    // uint32 input, return error
    {
        "Return_invalid_resource_path",         // input_access_token
        resource_path,                          // input_resource_path
        INTEGER,                                // input_format
        "100",                                  // input_resource_value
        CLOUD_CONNECT_ERR_INVALID_RESOURCE_PATH // expected_sd_bus_error_name
    },
    // very long path, string input, return success
    {
        "Return_Success",              // input_access_token
        "/55555555/44444444/33333333", // input_resource_path
        STRING,                        // input_format
        string_value.c_str(),          // input_resource_value
        "" /* not relevant */          // expected_sd_bus_error_name
    },
    // very long string input, return success
    {
        "Return_Success",                                                    // input_access_token
        resource_path,                                                       // input_resource_path
        STRING,                                                              // input_format
        "very_long_string_input_parameter_very_long_string_input_parameter", // input_resource_value
        "" /* not relevant */ // expected_sd_bus_error_name
    },
    // int64 maximal value input LLONG_MAX, return success
    {
        "Return_Success",      // input_access_token
        resource_path,         // input_resource_path
        INTEGER,               // input_format
        "9223372036854775807", // input_resource_value
        "" /* not relevant */  // expected_sd_bus_error_name
    },
    // int64 minimal value input LLONG_MIN, return success
    {
        "Return_Success",       // input_access_token
        resource_path,          // input_resource_path
        INTEGER,                // input_format
        "-9223372036854775807", // input_resource_value
        "" /* not relevant */   // expected_sd_bus_error_name
    },
};

static int AppThreadCb_validate_adapter_get_resources_values(AppThread* app_thread, void* user_data)
{
    TR_DEBUG_ENTER;
    assert(app_thread);
    assert(user_data);

    AdapterParameterizedData* adapter_param_data =
        static_cast<AdapterParameterizedData*>(user_data);
    const GetResourcesValues_entry& test_data =
        GetResourcesValues_test_array[adapter_param_data->test_array_index_];

    sd_bus_message* m_reply = nullptr;
    sd_bus_object_cleaner<sd_bus_message> reply_cleaner(m_reply, sd_bus_message_unref);

    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_object_cleaner<sd_bus_error> error_cleaner(&error, sd_bus_error_free);

    int test_result = TEST_SUCCESS;

    int r = sd_bus_call_method(app_thread->get_connection_handle(),
                               DBUS_CLOUD_SERVICE_NAME,
                               DBUS_CLOUD_CONNECT_OBJECT_PATH,
                               DBUS_CLOUD_CONNECT_INTERFACE_NAME,
                               DBUS_CC_GET_RESOURCES_VALUES_METHOD_NAME,
                               &error,
                               &m_reply,
                               "sa(sy)",
                               test_data.input_access_token,
                               1, /* number of array entries */
                               test_data.input_resource_path,
                               test_data.type);

    if (r < 0) {
        // message reply error gotten, compare to expected errors
        if (0 != strcmp(error.name, test_data.expected_sd_bus_error_name)) {

            TR_ERR("Actual error(%s) != Expected error(%s)",
                   error.name,
                   test_data.expected_sd_bus_error_name);
            set_test_result(test_result, TEST_FAILED_EXPECTED_RESULT_MISMATCH);
        }
        // check the error message
        if (std::string(test_data.input_access_token) == "Return_invalid_resource_path") {

            std::string expected_message("Get LWM2M resources failed: " +
                                         std::string(test_data.input_resource_path) + " : " +
                                         CloudConnectStatus_to_str(ERR_INVALID_RESOURCE_PATH));

            if (error.message != expected_message) {
                TR_ERR("Actual error message(%s) != Expected error message(%s)",
                       error.message,
                       expected_message.c_str());
                set_test_result(test_result, TEST_FAILED_EXPECTED_RESULT_MISMATCH);
            }
        }
    }
    else
    { // method reply gotten, read data type and value

        r = sd_bus_message_enter_container(m_reply, SD_BUS_TYPE_ARRAY, "(yv)");
        if (r < 0) {
            TR_ERR("sd_bus_message_enter_container failed(err=%d)", r);
            set_test_result(test_result, TEST_FAILED_SD_BUS_SYSTEM_CALL_FAILED);
        }
        r = sd_bus_message_enter_container(m_reply, SD_BUS_TYPE_STRUCT, "yv");
        if (r < 0) {
            TR_ERR("sd_bus_message_enter_container failed(err=%d)", r);
            set_test_result(test_result, TEST_FAILED_SD_BUS_SYSTEM_CALL_FAILED);
        }
        uint8_t type;
        r = sd_bus_message_read(m_reply, "y", &type);
        if (r < 0) {
            TR_ERR("sd_bus_message_read failed(err=%d)", r);
            set_test_result(test_result, TEST_FAILED_SD_BUS_SYSTEM_CALL_FAILED);
        }
        // read the data type
        if (type != test_data.type) {
            TR_ERR("Unexpected data type (%d) != Expected data type(%d)", type, test_data.type);
            set_test_result(test_result, TEST_FAILED_EXPECTED_RESULT_MISMATCH);
        }
        // read the data variant
        switch (type)
        {
        case STRING:
        {
            const char* value = nullptr;
            r = sd_bus_message_read(m_reply, "v", "s", &value);
            if (r < 0) {
                TR_ERR("sd_bus_message_read failed(err=%d)", r);
                set_test_result(test_result, TEST_FAILED_SD_BUS_SYSTEM_CALL_FAILED);
            }
            if (std::string(test_data.input_resource_value) != string(value)) {
                TR_ERR("Unexpected string value (%s) != Expected string value(%s)",
                       value,
                       test_data.input_resource_value);
                set_test_result(test_result, TEST_FAILED_EXPECTED_RESULT_MISMATCH);
            }
            break;
        }
        case INTEGER:
        {
            int64_t value = 0;
            r = sd_bus_message_read(m_reply, "v", "x", &value);
            if (r < 0) {
                TR_ERR("sd_bus_message_read failed(err=%d)", r);
                set_test_result(test_result, TEST_FAILED_SD_BUS_SYSTEM_CALL_FAILED);
            }
            size_t size = 0;
            int64_t expected_value = std::stoll(test_data.input_resource_value, &size);
            assert(size == strlen(test_data.input_resource_value));

            if (expected_value != value) {
                TR_ERR("Unexpected integer value (%" PRIx64 ") != Expected integer value(%" PRIx64
                       ")",
                       value,
                       expected_value);
                set_test_result(test_result, TEST_FAILED_EXPECTED_RESULT_MISMATCH);
            }
            break;
        }
        default:
        {
            TR_ERR("Unsupported data type (%d) != Expected data type(%d)", type, test_data.type);
            set_test_result(test_result, TEST_FAILED_EXPECTED_RESULT_MISMATCH);
        }
        }
    }

    // we stop adapter event loop from this thread instead of having one more additional thread
    DBusAdapter& adapter = adapter_param_data->adapater_;
    MblError adapter_finish_status = send_adapter_stop_message(&adapter);
    if (MblError::None != adapter_finish_status) {
        TR_ERR("send_adapter_stop_message(err=%s)", MblError_to_str(adapter_finish_status));
        set_test_result(test_result, TEST_FAILED_ADAPTER_METHOD_FAILED);
    }

    return test_result;
}

class ValidateGetResourcesValues : public testing::TestWithParam<int>
{
};
INSTANTIATE_TEST_CASE_P(AdapterParameterizedTest,
                        ValidateGetResourcesValues,
                        ::testing::Range(0, (int) GetResourcesValues_test_array.size(), 1));
TEST_P(ValidateGetResourcesValues, BasicMethodReply)
{
    GTEST_LOG_START_TEST;
    const GetResourcesValues_entry& test_data = GetResourcesValues_test_array[(size_t) GetParam()];
    MessageReplyTest_ResourceBroker ccrb_mock(
        test_data.type, test_data.input_resource_path, test_data.input_resource_value);
    DBusAdapter adapter(ccrb_mock);
    ccrb_mock.set_ipc_adapter(&adapter);

    ASSERT_EQ(adapter.init(), MblError::None);

    AdapterParameterizedData userdata(adapter, (size_t) GetParam());

    AppThread app_thread(AppThreadCb_validate_adapter_get_resources_values, &userdata);
    ASSERT_EQ(app_thread.create(), 0);

    MblError stop_status;
    ASSERT_EQ(adapter.run(stop_status), MblError::None);

    void* retval = nullptr;
    ASSERT_EQ(app_thread.join(&retval), 0);
    ASSERT_EQ((uintptr_t) retval, TEST_SUCCESS);
    ASSERT_EQ(adapter.deinit(), MblError::None);
}
