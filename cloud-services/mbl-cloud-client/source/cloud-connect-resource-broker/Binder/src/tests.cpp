/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <semaphore.h>
#include <systemd/sd-event.h>
#include <systemd/sd-bus.h>

#include "DBusAdapter.h"
#include "DBusAdapterMailbox.h"
#include "MblError.h"
#include "DBusMailboxMsg.h"
#include "DBusAdapterLowLevel_internal.h"
#include "DBusAdapterLowLevel.h"

#define DBUS_MAILBOX_WAIT_TIME_MS 100000

//TODO : replace this with blocking query
#define SLEEP_MS(time_to_sleep_im_ms) usleep(1000 * time_to_sleep_im_ms)

using MblError = mbl::MblError;
extern "C" DBusAdapterLowLevelContext *DBusAdapterLowLevel_GetContext();

TEST(DBusAdapterMailBox_a, InitDeinit)
{
    mbl::DBusAdapterMailbox mailbox;

    ASSERT_EQ(mailbox.init(), MblError::None);
    ASSERT_EQ(mailbox.deinit(), MblError::None);
}

TEST(DBusAdapterMailBox_a, SendReceiveRawMessagePtr_SingleThread)
{
    mbl::DBusAdapterMailbox mailbox;
    mbl::DBusMailboxMsg write_msg;
    mbl::DBusMailboxMsg read_msg;
    uintptr_t msg_ptr_as_int;
    std::string str("Hello1 Hello2 Hello3");

    memset(&write_msg, 0, sizeof(write_msg));
    memset(&read_msg, 0, sizeof(read_msg));
    write_msg.type_ = mbl::DBusMailboxMsg::MsgType::RAW_DATA;
    read_msg.payload_len_ = str.length();
    strncpy(
        write_msg.payload_.raw.bytes,
        str.c_str(),
        str.length());

    ASSERT_EQ(mailbox.init(), 0);

    // send / receive / compare 100 times
    for (auto i = 0; i < 100; i++)
    {
        ASSERT_EQ(mailbox.send_msg(write_msg, DBUS_MAILBOX_WAIT_TIME_MS), 0);
        ASSERT_EQ(mailbox.receive_msg(read_msg, DBUS_MAILBOX_WAIT_TIME_MS), 0);
        write_msg = read_msg;
        ASSERT_EQ(memcmp(&write_msg, &read_msg, sizeof(mbl::DBusMailboxMsg)), 0);
    }

    ASSERT_EQ(mailbox.deinit(), 0);
}

static void *reader_thread_start(void *mailbox)
{
    mbl::DBusAdapterMailbox *mailbox_ = static_cast<mbl::DBusAdapterMailbox *>(mailbox);
    mbl::DBusMailboxMsg output_msg;
    uint64_t expected_sequence_num = 0;

    for (char ch = 'A'; ch < 'Z' + 1; ++ch, ++expected_sequence_num)
    {
        if (mailbox_->receive_msg(output_msg, DBUS_MAILBOX_WAIT_TIME_MS) != 0)
        {
            pthread_exit((void *)-1);
        }
        if ((output_msg.type_ != mbl::DBusMailboxMsg::MsgType::RAW_DATA) ||
            (output_msg.payload_len_ != 1) ||
            (output_msg.payload_.raw.bytes[0] != ch) ||
            (output_msg._sequence_num != expected_sequence_num))
        {
            pthread_exit((void *)-1);
        }
    }
    pthread_exit((void *)0);
}

static void *writer_thread_start(void *mailbox)
{
    mbl::DBusMailboxMsg input_message;
    mbl::DBusAdapterMailbox *mailbox_ = static_cast<mbl::DBusAdapterMailbox *>(mailbox);

    memset(&input_message, 0, sizeof(input_message));
    input_message.type_ = mbl::DBusMailboxMsg::MsgType::RAW_DATA;
    input_message.payload_len_ = 1;

    for (char ch = 'A'; ch < 'Z' + 1; ++ch)
    {
        input_message.payload_.raw.bytes[0] = ch;
        if (mailbox_->send_msg(input_message, DBUS_MAILBOX_WAIT_TIME_MS) != 0)
        {
            pthread_exit((void *)-1);
        }
    }

    pthread_exit((void *)0);
}

TEST(DBusAdapterMailBox_a, SendReceiveRawMessage_MultiThread)
{
    pthread_t writer_tid, reader_tid;
    mbl::DBusAdapterMailbox mailbox;
    void *retval;

    ASSERT_EQ(mailbox.init(), 0);
    ASSERT_EQ(pthread_create(&writer_tid, NULL, reader_thread_start, &mailbox), 0);
    ASSERT_EQ(pthread_create(&reader_tid, NULL, writer_thread_start, &mailbox), 0);
    ASSERT_EQ(pthread_join(writer_tid, &retval), 0);
    ASSERT_EQ((uintptr_t)retval, 0);
    ASSERT_EQ(pthread_join(reader_tid, &retval), 0);
    ASSERT_EQ((uintptr_t)retval, 0);
    ASSERT_EQ(mailbox.deinit(), 0);
}

class DBusAdapeterLowLevel_b : public ::testing::Test
{
  protected:
    DBusAdapeterLowLevel_b()
    {
        // set-up work for each test here.

        //This is a fast dummy initialization for testing
        callbacks.register_resources_async_callback = (int (*)(uintptr_t, const char *))1;
        callbacks.deregister_resources_async_callback = (int (*)(uintptr_t, const char *))2;
        callbacks.received_message_on_mailbox_callback = (int (*)(const int, void *))3;
    }

    ~DBusAdapeterLowLevel_b() override
    {
        // clean-up work that doesn't throw exceptions here.
    }

    // If the constructor and destructor are not enough for setting up
    // and cleaning up each test, you can define the following methods:
    void SetUp() override
    {
        // Code here will be called immediately after the constructor (right before each test).
        ASSERT_EQ(DBusAdapterLowLevel_init(&callbacks), 0);
    }

    void TearDown() override
    {
        // Code here will be called immediately after each test (right before the destructor).
        ASSERT_EQ(DBusAdapterLowLevel_deinit(), 0);
    }

    // Objects declared here can be used by all tests in the test case for this class.
    DBusAdapterCallbacks callbacks;
};

TEST_F(DBusAdapeterLowLevel_b, init_deinit)
{
    // empty test body
}

TEST_F(DBusAdapeterLowLevel_b, run_stop_with_self_request)
{
    DBusAdapterLowLevelContext *ctx = DBusAdapterLowLevel_GetContext();
    const int event_exit_code = 0xFFFEEEAA;

    // send my self exit signal before I enter the loop, expect the event_exit_code
    // usually we will send self stop from a callback and not this wey.
    // this is a test, so that's the easiest way to check (simulating a callback at this location)
    ASSERT_EQ(DBusAdapterLowLevel_event_loop_request_stop(event_exit_code), 0);

    // expect exit code to be event_exit_code since that what was sent
    ASSERT_EQ(DBusAdapterLowLevel_event_loop_run(), event_exit_code);
}

TEST(DBusAdapeter_c, init_deinit)
{
    mbl::DBusAdapter adapter_;
    ASSERT_EQ(adapter_.init(), mbl::MblError::None);
    ASSERT_EQ(adapter_.deinit(), mbl::MblError::None);
}

static void *mbl_cloud_client_thread(void *adapter_)
{
    mbl::DBusAdapter *adapter = static_cast<mbl::DBusAdapter *>(adapter_);
    MblError status;

    status = adapter->init();
    if (status != mbl::MblError::None)
    {
        pthread_exit((void *)status);
    }

    status = adapter->run();
    if (status != mbl::MblError::None)
    {
        pthread_exit((void *)status);
    }

    status = adapter->deinit();
    if (status != mbl::MblError::None)
    {
        pthread_exit((void *)status);
    }

    pthread_exit((void *)0);
}

TEST(DBusAdapeter_c, run_stop_with_external_exit_msg)
{
    mbl::DBusAdapter adapter;
    pthread_t tid;
    void *retval;

    //start stop 10 times
    for (auto i = 0; i < 10; i++)
    {
        ASSERT_EQ(pthread_create(&tid, NULL, mbl_cloud_client_thread, &adapter), 0);

        // TODO :replace with blocking query
        // do not use semaphoeres - it's only a test - sleep for 10 millisec
        SLEEP_MS(100000);

        ASSERT_EQ(adapter.stop(), mbl::MblError::None);

        ASSERT_EQ(pthread_join(tid, &retval), 0);
        ASSERT_EQ((uintptr_t)retval, 0);
        printf("%d", i);
    }
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}