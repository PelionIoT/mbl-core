/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <semaphore.h>

#include <gtest/gtest.h>

#include "DBusAdapter.h"
#include "DBusAdapterImpl.h"
#include "DBusAdapterMailbox.h"
#include "MblError.h"
#include "DBusMailboxMsg.h"
#include "DBusAdapterService.h"
#include "DBusAdapterTimer.h"

using namespace mbl;

#define SLEEP_MS(time_to_sleep_im_ms) usleep(1000 * time_to_sleep_im_ms)
#define DBUS_MAILBOX_MAX_WAIT_TIME_MS 10

///////////////////////////////////////////////////////////////////////////
////////////////////////////INFRA//////////////////////////////////////////
#define TESTER_VALIDATE_EQ(a, b) if (a != b) return MblError::DBusErr_Temporary
class DBusAdapterTester
{
    public:

        DBusAdapterTester(DBusAdapter &adapter) : 
            adapter_(adapter) {};

        MblError validate_deinitialized_adapter();
        MblError event_loop_request_stop(DBusAdapterStopStatus  stop_status);
        MblError event_loop_run(
            DBusAdapterStopStatus &stop_status, DBusAdapterStopStatus expected_stop_status);
        sd_event* get_event_loop_handle();

        //use this call only if calling thread is the one to initialize the adapter!
        int add_event_defer(sd_event_handler_t handler,void *userdata);


        DBusAdapter &adapter_;
        sd_event_source *source_;
};

MblError DBusAdapterTester::validate_deinitialized_adapter()
{
    TESTER_VALIDATE_EQ(adapter_.impl_->state_, DBusAdapter::DBusAdapterImpl::State::UNINITALIZED); 
    TESTER_VALIDATE_EQ(adapter_.impl_->pending_messages_.empty(), true);
    TESTER_VALIDATE_EQ(adapter_.impl_->event_loop_handle_, nullptr);
    TESTER_VALIDATE_EQ(adapter_.impl_->connection_handle_, nullptr);
    TESTER_VALIDATE_EQ(adapter_.impl_->connection_slot_, nullptr);
    TESTER_VALIDATE_EQ(adapter_.impl_->event_source_pipe_, nullptr);
    TESTER_VALIDATE_EQ(adapter_.impl_->unique_name_, nullptr);
    TESTER_VALIDATE_EQ(adapter_.impl_->service_name_, nullptr);
    return MblError::None;
}

MblError DBusAdapterTester::event_loop_request_stop(DBusAdapterStopStatus  stop_status)
{
    TESTER_VALIDATE_EQ(adapter_.impl_->event_loop_request_stop(stop_status), MblError::None);
    return MblError::None;
}

MblError DBusAdapterTester::event_loop_run(
    DBusAdapterStopStatus &stop_status, DBusAdapterStopStatus expected_stop_status)
{
    TESTER_VALIDATE_EQ(adapter_.impl_->event_loop_run(stop_status), MblError::None);
    TESTER_VALIDATE_EQ(stop_status, expected_stop_status);    
    return MblError::None;
}

sd_event* DBusAdapterTester::get_event_loop_handle()
{
    return adapter_.impl_->event_loop_handle_;
}

int DBusAdapterTester::add_event_defer(
    sd_event_handler_t handler, 
    void *userdata)
{
    return sd_event_add_defer(
        adapter_.impl_->event_loop_handle_,
        &source_,
        handler,
        userdata);
}
///////////////////////////////////////////////////////////////////////////


TEST(DBusAdapterMailBox1, InitDeinit)
{
    DBusAdapterMailbox mailbox;
    ASSERT_EQ(mailbox.init(), MblError::None);
    ASSERT_EQ(mailbox.deinit(), MblError::None);
}

TEST(DBusAdapterService1,init_get_deinit)
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
        ASSERT_EQ(mailbox.send_msg(write_msg, DBUS_MAILBOX_MAX_WAIT_TIME_MS), 0);
        ASSERT_EQ(mailbox.receive_msg(read_msg, DBUS_MAILBOX_MAX_WAIT_TIME_MS), 0);
        write_msg = read_msg;
        ASSERT_EQ(memcmp(&write_msg, &read_msg, sizeof(DBusMailboxMsg)), 0);
    }

    ASSERT_EQ(mailbox.deinit(), 0);
}


static void *reader_thread_start(void *mailbox)
{
    DBusAdapterMailbox *mailbox_ = static_cast<DBusAdapterMailbox *>(mailbox);
    DBusMailboxMsg output_msg;
    uint64_t expected_sequence_num = 0;

    for (char ch = 'A'; ch < 'Z' + 1; ++ch, ++expected_sequence_num)
    {
        if (mailbox_->receive_msg(output_msg, DBUS_MAILBOX_MAX_WAIT_TIME_MS) != 0)
        {
            pthread_exit((void *)-1);
        }
        if ((output_msg.type_ != DBusMailboxMsg::MsgType::RAW_DATA) ||
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
    DBusMailboxMsg input_message;
    DBusAdapterMailbox *mailbox_ = static_cast<DBusAdapterMailbox *>(mailbox);

    memset(&input_message, 0, sizeof(input_message));
    input_message.type_ = DBusMailboxMsg::MsgType::RAW_DATA;
    input_message.payload_len_ = 1;

    for (char ch = 'A'; ch < 'Z' + 1; ++ch)
    {
        input_message.payload_.raw.bytes[0] = ch;
        if (mailbox_->send_msg(input_message, DBUS_MAILBOX_MAX_WAIT_TIME_MS) != 0)
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

    ASSERT_EQ(mailbox.init(), 0);
    ASSERT_EQ(pthread_create(&writer_tid, NULL, reader_thread_start, &mailbox), 0);
    ASSERT_EQ(pthread_create(&reader_tid, NULL, writer_thread_start, &mailbox), 0);
    ASSERT_EQ(pthread_join(writer_tid, &retval), 0);
    ASSERT_EQ((uintptr_t)retval, 0);
    ASSERT_EQ(pthread_join(reader_tid, &retval), 0);
    ASSERT_EQ((uintptr_t)retval, 0);
    ASSERT_EQ(mailbox.deinit(), 0);
}

TEST(DBusAdapeter1, init_deinit)
{
    DBusAdapter adapter;
    DBusAdapterTester tester(adapter);
    
    for (auto i = 0; i < 10; ++i){
        ASSERT_EQ(adapter.init(), MblError::None);        
        ASSERT_EQ(adapter.deinit(), MblError::None);        
        tester.validate_deinitialized_adapter();
    }
}

/*
static int event_loop_request_stop(sd_event_source *s, void *userdata)
{
    DBusAdapterTester *tester = static_cast<DBusAdapterTester*>(userdata);
    return tester->event_loop_request_stop(DBusAdapterStopStatus::DBUS_ADAPTER_STOP_STATUS_NO_ERROR);
}

TEST(DBusAdapeter1, run_stop_with_self_request)
{
    DBusAdapter adapter;    
    DBusAdapterStopStatus stop_status = DBusAdapterStopStatus::DBUS_ADAPTER_STOP_STATUS_NO_ERROR;
    DBusAdapterTester tester(adapter);
    sd_event_source *source;

    ASSERT_EQ(adapter.init(), MblError::None);

    sd_event_source *s;
    sd_event_exit(tester.get_event_loop_handle(), DBusAdapterStopStatus::DBUS_ADAPTER_STOP_STATUS_NO_ERROR);
    //ASSERT_GE(tester.add_event_defer(event_loop_request_stop, &tester), 0);
        //sd_event *ev = tester.get_event_loop_handle();
    //sd_event_loop(ev);

    // expect exit code to be event_exit_code since that what was sent
    ASSERT_EQ(tester.event_loop_run(
        stop_status, DBusAdapterStopStatus::DBUS_ADAPTER_STOP_STATUS_NO_ERROR), 0);

    ASSERT_EQ(adapter.deinit(), MblError::None);
}
*/


static void *mbl_cloud_client_thread(void *adapter_)
{
    DBusAdapter *adapter = static_cast<DBusAdapter *>(adapter_);
    MblError status;
    DBusAdapterStopStatus stop_status;

    status = adapter->init();
    if (status != MblError::None){
        pthread_exit((void *)status);
    }

    status = adapter->run(stop_status);
    if (status != MblError::None){
        pthread_exit((void *)status);
    }
    if (stop_status != DBusAdapterStopStatus::DBUS_ADAPTER_STOP_STATUS_NO_ERROR){
        pthread_exit((void *)MblError::DBusErr_Temporary);
    }

    status = adapter->deinit();
    if (status != MblError::None){
        pthread_exit((void *)status);
    }

    pthread_exit((void *)0);
}


TEST(DBusAdapeter_c, run_stop_with_external_exit_msg)
{
    DBusAdapter adapter;
    pthread_t tid;
    void *retval;

    //start stop 10 times
    for (auto i = 0; i < 10; i++)
    {
        ASSERT_EQ(pthread_create(&tid, NULL, mbl_cloud_client_thread, &adapter), 0);

        // TODO :replace with blocking query
        // do not use semaphores - it's only a test - sleep for 10 millisec
        SLEEP_MS(10); 

        ASSERT_EQ(adapter.stop(DBusAdapterStopStatus::DBUS_ADAPTER_STOP_STATUS_NO_ERROR), MblError::None);

        ASSERT_EQ(pthread_join(tid, &retval), 0);
        ASSERT_EQ((uintptr_t)retval, MblError::None);
    }
}


int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}