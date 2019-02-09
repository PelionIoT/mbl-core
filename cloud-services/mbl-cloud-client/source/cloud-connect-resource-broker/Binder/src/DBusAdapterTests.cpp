/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <semaphore.h>
#include <pthread.h>

#include <gtest/gtest.h>

#include "DBusAdapter.h"
#include "DBusAdapter_Impl.h"
#include "DBusAdapterMailbox.h"
#include "MblError.h"
#include "DBusMailboxMsg.h"
#include "DBusAdapterService.h"
#include "TestInfra_DBusAdapterTester.h"

#include "TestInfra_AppThread.h"
#include "TestInfra_Common.h"

using namespace mbl;

static sem_t s_sem;

TEST(DBusAdapterMailBox1, InitDeinit)
{
    DBusAdapterMailbox mailbox;
    ASSERT_EQ(mailbox.init(), MblError::None);
    ASSERT_EQ(mailbox.deinit(), MblError::None);
}

TEST(DBusAdapterService1, init_get_deinit)
{
    // initialize callback to non-zero. check null/non-null userdata
    uintptr_t none_zero_val = 1;
    IncomingDataCallback callback = (IncomingDataCallback)none_zero_val;

    ASSERT_EQ(DBusAdapterService_init(callback), 0);
    ASSERT_EQ(DBusAdapterService_init(callback), 0);
    ASSERT_NE(DBusAdapterService_get_service_vtable(), nullptr);
    ASSERT_EQ(DBusAdapterService_deinit(), 0);
}

TEST(DBusAdapterMailBox1, SendReceiveRawMessagePtr_SingleThread)
{
    DBusAdapterMailbox mailbox;
    DBusMailboxMsg write_msg;
    DBusMailboxMsg read_msg;
    uintptr_t msg_ptr_as_int;
    std::string str("Hello1 Hello2 Hello3");

    memset(&write_msg, 0, sizeof(write_msg));
    memset(&read_msg, 0, sizeof(read_msg));
    write_msg.type_ = DBusMailboxMsg::MsgType::RAW_DATA;
    read_msg.payload_len_ = str.length();
    strncpy(
        write_msg.payload_.raw.bytes,
        str.c_str(),
        str.length());

    ASSERT_EQ(mailbox.init(), 0);

    // send / receive / compare 100 times
    for (auto i = 0; i < 100; i++)
    {
        ASSERT_EQ(mailbox.send_msg(write_msg, TI_DBUS_MAILBOX_MAX_WAIT_TIME_MS), 0);
        ASSERT_EQ(mailbox.receive_msg(read_msg, TI_DBUS_MAILBOX_MAX_WAIT_TIME_MS), 0);
        write_msg = read_msg;
        ASSERT_EQ(memcmp(&write_msg, &read_msg, sizeof(DBusMailboxMsg)), 0);
    }

    ASSERT_EQ(mailbox.deinit(), 0);
}

static void *SendReceiveRawMessage_MultiThread_reader_thread_start(void *mailbox)
{
    assert(mailbox);
    DBusAdapterMailbox *mailbox_ = static_cast<DBusAdapterMailbox *>(mailbox);
    DBusMailboxMsg output_msg;
    uint64_t expected_sequence_num = 0;

    for (char ch = 'A'; ch < 'Z' + 1; ++ch, ++expected_sequence_num)
    {
        if (mailbox_->receive_msg(output_msg, TI_DBUS_MAILBOX_MAX_WAIT_TIME_MS) != 0)
        {
            pthread_exit((void *)-1003);
        }
        if ((output_msg.type_ != DBusMailboxMsg::MsgType::RAW_DATA) ||
            (output_msg.payload_len_ != 1) ||
            (output_msg.payload_.raw.bytes[0] != ch) ||
            (output_msg._sequence_num != expected_sequence_num))
        {
            pthread_exit((void *)-1004);
        }
    }
    pthread_exit((void *)0);
}

static void *SendReceiveRawMessage_MultiThread_writer_thread_start(void *mailbox)
{
    assert(mailbox);
    DBusMailboxMsg input_message;
    DBusAdapterMailbox *mailbox_ = static_cast<DBusAdapterMailbox *>(mailbox);

    memset(&input_message, 0, sizeof(input_message));
    input_message.type_ = DBusMailboxMsg::MsgType::RAW_DATA;
    input_message.payload_len_ = 1;

    for (char ch = 'A'; ch < 'Z' + 1; ++ch)
    {
        input_message.payload_.raw.bytes[0] = ch;
        if (mailbox_->send_msg(input_message, TI_DBUS_MAILBOX_MAX_WAIT_TIME_MS) != 0)
        {
            pthread_exit((void *)-1);
        }
    }

    pthread_exit((void *)0);
}

TEST(DBusAdapterMailBox1, SendReceiveRawMessage_MultiThread)
{
    pthread_t writer_tid, reader_tid;
    DBusAdapterMailbox mailbox;
    void *retval;

    for (auto i = 0; i < 100; i++)
    {
        ASSERT_EQ(mailbox.init(), 0);
        ASSERT_EQ(pthread_create(&writer_tid, NULL, SendReceiveRawMessage_MultiThread_reader_thread_start, &mailbox), 0);
        ASSERT_EQ(pthread_create(&reader_tid, NULL, SendReceiveRawMessage_MultiThread_writer_thread_start, &mailbox), 0);
        ASSERT_EQ(pthread_join(writer_tid, &retval), 0);
        ASSERT_EQ((uintptr_t)retval, 0);
        ASSERT_EQ(pthread_join(reader_tid, &retval), 0);
        ASSERT_EQ((uintptr_t)retval, 0);
        ASSERT_EQ(mailbox.deinit(), 0);
    }
}

TEST(DBusAdapeter1, init_deinit)
{
    DBusAdapter adapter;
    DBusAdapterTester tester(adapter);

    for (auto i = 0; i < 10; ++i)
    {
        ASSERT_EQ(adapter.init(), MblError::None);
        ASSERT_EQ(adapter.deinit(), MblError::None);
        tester.validate_deinitialized_adapter();
    }
}

static int event_loop_request_stop(sd_event_source *s, void *userdata)
{
    assert(userdata);
    DBusAdapterTester *tester = static_cast<DBusAdapterTester*>(userdata);
    return tester->event_loop_request_stop(MblError::None);
}


TEST(DBusAdapeter1, run_stop_with_self_request)
{
    DBusAdapter adapter;    
    MblError stop_status = MblError::DBusStopStatusErrorInternal;
    DBusAdapterTester tester(adapter);

    ASSERT_EQ(adapter.init(), MblError::None);
    ASSERT_GE(tester.add_event_defer(event_loop_request_stop, &tester), 0);
    EXPECT_EQ(tester.event_loop_run(
        stop_status, MblError::None), 0);
    ASSERT_EQ(adapter.deinit(), MblError::None);
}


static void *mbl_cloud_client_thread(void *adapter)
{
    assert(adapter);
    DBusAdapter *adapter_ = static_cast<DBusAdapter *>(adapter);
    MblError status;
    MblError stop_status;

    status = adapter_->init();
    if (status != MblError::None)
    {
        pthread_exit((void *)status);
    }

    if (sem_post(&s_sem) != 0)
    {
        pthread_exit((void *)-1005);
    }
    status = adapter_->run(stop_status);
    if (status != MblError::None)
    {
        pthread_exit((void *)status);
    }
    if (stop_status != MblError::None)
    {
        pthread_exit((void *)MblError::DBusErr_Temporary);
    }

    status = adapter_->deinit();
    if (status != MblError::None)
    {
        pthread_exit((void *)status);
    }

    pthread_exit((void *)0);
}


TEST(DBusAdapeter_c, run_stop_with_external_exit_msg)
{
    DBusAdapter adapter;
    pthread_t tid;
    void *retval;
    
    // i'm going to wait on a semaphore
    ASSERT_EQ(sem_init(&s_sem, 0, 0), 0);

    //start stop 10 times
    for (auto i = 0; i < 10; i++)
    {
        ASSERT_EQ(pthread_create(&tid, NULL, mbl_cloud_client_thread, &adapter), 0);
        ASSERT_EQ(sem_wait(&s_sem), 0);
        ASSERT_EQ(adapter.stop(MblError::None), MblError::None);
        ASSERT_EQ(pthread_join(tid, &retval), 0);
        ASSERT_EQ((uintptr_t)retval, MblError::None);
    }

    ASSERT_EQ(sem_destroy(&s_sem), 0);
}

static int AppThreadCb_validate_service_exist(AppThread *app_thread, void *user_data)
{
    assert(app_thread);
    AppThread *app_thread_ = static_cast<AppThread *>(app_thread);
    return sd_bus_request_name(app_thread_->connection_handle_, DBUS_CLOUD_SERVICE_NAME, 0);
}

TEST(DBusAdapeter_c, ValidateServiceExist)
{
    DBusAdapter adapter;
    AppThread app_thread(AppThreadCb_validate_service_exist, nullptr);
    void *retval = nullptr;

    ASSERT_EQ(adapter.init(), MblError::None);        
    ASSERT_EQ(app_thread.create(), 0);
    ASSERT_EQ(app_thread.join(&retval), 0);
    ASSERT_EQ((uintptr_t)retval, -EEXIST);
    ASSERT_EQ(adapter.deinit(), MblError::None);
}

int main(int argc, char **argv)
{    
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
