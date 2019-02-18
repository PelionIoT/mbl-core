/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "CloudConnectTrace.h"
#include "DBusAdapter.h"
#include "DBusAdapter_Impl.h"
#include "Mailbox.h"
#include "MblError.h"
#include "MailboxMsg.h"
#include "DBusService.h"
#include "EventManager.h"
#include "SelfEvent.h"
#include "ResourceBroker.h"
#include "CloudConnectTypes.h"

#include "TestInfra_DBusAdapterTester.h"
#include "TestInfraAppThread.h"
#include "TestInfra.h"

#include <gtest/gtest.h>

#include <semaphore.h>
#include <pthread.h>
#include <memory>
#include <random>
#include <stdlib.h>
#include <vector>
#include <functional>
#include <algorithm> 

#define TRACE_GROUP "ccrb-dbus-gtest"

using namespace mbl;

/////////////////////////////////////////////////////////////////////////////
//////////////////// MailBox ////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/**
 * @brief Fixture class for all Mailbox tests
 * 
 */
class MailBoxTestFixture : public ::testing::Test 
{
public:
    const int NUM_ITERATIONS = 100;

    MailBoxTestFixture() : mailbox("")  {}

    /**
     * @brief generate a binary string ' random_str_out' of 0/1 of length 'length'
     */
    void generate_random_binary_string(std::string& random_str_out, int length)
    {
        TR_DEBUG("Enter");
        random_str_out.empty();
        for (auto i = 0; i < length; ++i){
            random_str_out += binary_chars_[rand() % binary_chars_.size()];
        }
    }

    // thread starting functions for test 'send_rcv_msg_single_thread'
    static void* reader_thread_start(void*);
    static void* writer_thread_start(void*);
    Mailbox mailbox;

private:
    std::vector<char> binary_chars_{'0','1'};
};

/**
 * @brief Positive Test - init/deinit a mail box NUM_ITERATIONS times
 * 
 */
TEST_F(MailBoxTestFixture, init_deinit)
{
    GTEST_LOG_START_TEST;

    for (auto i = 0; i < NUM_ITERATIONS; i++){        
        ASSERT_EQ(mailbox.init(), MblError::None);
        ASSERT_EQ(mailbox.deinit(), MblError::None);
    }
}

/**
 * @brief Positive Test - send and receive a raw message MailboxMsg::MsgType::RAW_DATA 
 * times with random string of inceasing length. 
 * validate string match at receiving side and length bytes_len_.
 */
TEST_F(MailBoxTestFixture, send_rcv_msg_single_thread)
{
    GTEST_LOG_START_TEST;
    ASSERT_EQ(mailbox.init(), 0);

    // generate / send / receive & compare MsgRaw::MAX_SIZE times
    for (auto i = 1; i < MailboxMsg::MsgPayload::MsgRaw::MAX_SIZE; i++)
    {
        MailboxMsg::MsgPayload send_payload;
        std::string random_str;

        // generate a random binary string of size i
        memset(&send_payload, 0, sizeof(send_payload));
        generate_random_binary_string(random_str, i);

        //copy string to the send_payload rae message part
        strncpy(
            (char*)send_payload.raw_.bytes,
            random_str.c_str(),
            random_str.length());

        //create and send the message
        MailboxMsg msg_to_send(MailboxMsg::MsgType::RAW_DATA, send_payload, random_str.length());  
        ASSERT_EQ(mailbox.send_msg(msg_to_send), 0);

        //receive the message into a pair
        auto ret_pair = mailbox.receive_msg();

        //validate success to receive
        ASSERT_EQ(ret_pair.first, MblError::None);

        //check that sent data equal received data
        ASSERT_EQ(memcmp(&msg_to_send.get_payload(),
                        &ret_pair.second.get_payload(), 
                        random_str.length()) ,0);

        //validate length
        ASSERT_EQ(msg_to_send.get_payload_len(), ret_pair.second.get_payload_len());

        //validate sequence number
        ASSERT_EQ(msg_to_send.get_sequence_num(), ret_pair.second.get_sequence_num());
    }

    ASSERT_EQ(mailbox.deinit(), 0);
}

void* MailBoxTestFixture::reader_thread_start(void *mailbox)
{
    TR_DEBUG("Enter");
    assert(mailbox);
    uint64_t last_sequence_num = 0;    
    Mailbox *mailbox_in_ = static_cast<Mailbox *>(mailbox);
        
    for (char ch = 'A'; ch < 'Z' + 1; ++ch)
    {   
        //get message into pair, validate success
        auto ret_pair = mailbox_in_->receive_msg();        
        if (ret_pair.first != MblError::None){
            pthread_exit((void *)-1003);
        }

        // validate type, length actual data received
        if ((ret_pair.second.get_type() != MailboxMsg::MsgType::RAW_DATA) ||
            (ret_pair.second.get_payload_len() != 1) ||
            (ret_pair.second.get_payload().raw_.bytes[0] != ch))
        {
            pthread_exit((void *)-1004);
        }

        //check that sequence number increase every iteration
        if ((last_sequence_num != 0) && 
            (ret_pair.second.get_sequence_num() != (last_sequence_num + 1)))
        {
            pthread_exit((void *)-1005);
        }
        last_sequence_num = ret_pair.second.get_sequence_num();
    }
    pthread_exit((void *)0);
}

void* MailBoxTestFixture::writer_thread_start(void *mailbox)
{
    TR_DEBUG("Enter");
    assert(mailbox);
    Mailbox *mailbox_in_ = static_cast<Mailbox *>(mailbox);
    MailboxMsg::MsgPayload payload;     

    for (char ch = 'A'; ch < 'Z' + 1; ++ch)
    {
        // fill payload and send message
        payload.raw_.bytes[0] = ch;
        MailboxMsg write_msg(MailboxMsg::MsgType::RAW_DATA, payload, 1); 
        if (mailbox_in_->send_msg(write_msg) != 0)
        {
            pthread_exit((void *)-1);
        }
    }

    pthread_exit((void *)0);
}

/**
 * @brief Construct a new test f object
 * THis test start a reader and writer threads. THe writer thread send a single chaning character
 * as a raw message into the mailbox, and the reader receive the message, and validates the data.
 * THis repeats A to Z by both threads, and 100 times more from the outer main thread loop (total
 * 2600 messages sent/received).
 */
TEST_F(MailBoxTestFixture, send_rcv_raw_message_multi_thread)
{
    GTEST_LOG_START_TEST;
    pthread_t writer_tid, reader_tid;
    void *retval;

    for (auto i = 0; i < NUM_ITERATIONS; i++)
    {   
        // in this loop : init mailbox, start both threads and later on join them, check 0 success 
        // status code for both threads and deinit mailbox
        ASSERT_EQ(mailbox.init(), 0);
        ASSERT_EQ(pthread_create(&writer_tid,
                                NULL, 
                                MailBoxTestFixture::writer_thread_start,
                                &mailbox), 0);

        //wait 5ms to allow writer to write something.
        ASSERT_EQ(usleep(5 * 1000), 0);
        ASSERT_EQ(pthread_create(&reader_tid,
                                NULL,
                                MailBoxTestFixture::reader_thread_start,
                                &mailbox), 0);
        ASSERT_EQ(pthread_join(writer_tid, &retval), 0);
        ASSERT_EQ((uintptr_t)retval, 0);
        ASSERT_EQ(pthread_join(reader_tid, &retval), 0);
        ASSERT_EQ((uintptr_t)retval, 0);
        ASSERT_EQ(mailbox.deinit(), 0);
    }
}

///////////////////////////////////////////////////////////////////////////
////////////////// Immidiate SelfEvent ////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

/**
 * @brief Fixture class for all EventManager/Selfevent tests
 * These tests are all single threaded
 */
class EventManagerTestFixture : public ::testing::Test 
{
public:
    // this callback is used in the basic test with no adapter
    static MblError basic_no_adapter_callback(sd_event_source *s, const SelfEvent *ev);
    static const int START_VAL = 10;
    static sd_event *event_loop_handle_;
    static const int NUM_ITERATIONS = 10;    

    void SetUp() override 
    {
        TR_DEBUG("Enter");
        ASSERT_GE(sd_event_default(&event_loop_handle_), 0);                             
    }

    void TearDown() override
    {
        TR_DEBUG("Enter");
        sd_event_unref(event_loop_handle_);
    }
};
sd_event* EventManagerTestFixture::event_loop_handle_ = nullptr;

MblError EventManagerTestFixture::basic_no_adapter_callback(sd_event_source *s, const SelfEvent *ev)
{    
    UNUSED(s);
    TR_DEBUG("Enter");
    const SelfEvent::EventDataType &event_data = ev->get_data();
    OneSetMblError result;    
    static int iteration = 0;
    static std::vector<bool> event_arrive_flag(NUM_ITERATIONS, true);

    //validate data as expected - events might arrived unordered
    //get the first byte and then check incremental order.
    //validate all values arrived using a vector
    char start_val = event_data.raw.bytes[0];
    if ((start_val >= (START_VAL + NUM_ITERATIONS)) ||
        (start_val < START_VAL))
    {
        sd_event_exit(event_loop_handle_, MblError::DBA_InvalidValue);
    }
    if (false == event_arrive_flag[start_val]){
        //arrived already
        sd_event_exit(event_loop_handle_, MblError::DBA_InvalidValue);
    }
    event_arrive_flag[start_val-START_VAL] = false;
    for (unsigned long i = 1; i < sizeof(event_data.raw.bytes); i++){

        if (event_data.raw.bytes[i] != (start_val + (int)i)){
            sd_event_exit(event_loop_handle_, MblError::DBA_InvalidValue);
            break;
        }
    }   

    //send exit if finished all iterations
    iteration++;
    if (iteration >= NUM_ITERATIONS){
        //check that all events arrived
        for (bool elem : event_arrive_flag){
            if (elem){
                sd_event_exit(event_loop_handle_, MblError::DBA_InvalidValue);
            }
        }
        //all events down! call exit to exit default loop with success!
        sd_event_exit(event_loop_handle_, MblError::None);
    }

    return result.get();
}

/**
 * @brief
 * In this test the main test body sends NUM_ITERATIONS timers an event to itself, the event data is 
 * changing between events. (starting from START_VAL at i=0 for first byte and increasing by 1). 
 * Since the callback does not  send events but only handle them, all events sent in a loop and then
 * outside of loop test body enters the sd-event loop.
 * after NUM_ITERATIONS the callback function sends an exist request.
 */
TEST_F(EventManagerTestFixture, basic_no_adapter)
{
    GTEST_LOG_START_TEST;
    SelfEvent::EventDataType event_data = { 0 };
    uint64_t out_event_id = 0;
    EventManager event_manager_;
    
    // initialize event manager
    event_manager_.init();

    for (int i = 0; i < NUM_ITERATIONS; i++){                
        TR_DEBUG("Start iteration %d out of %d", i, NUM_ITERATIONS);

        // Fill the raw data event type up to maximum with increasing values starting from START_VAL
        std::iota(
            event_data.raw.bytes, 
            event_data.raw.bytes + sizeof(event_data.raw.bytes), 
            START_VAL+i);

        // send the message and enter the loop to start dispatching events
        TR_DEBUG_POINT;
        ASSERT_EQ(
            event_manager_.send_event_immediate(
                event_data, 
                sizeof(event_data.raw),
                SelfEvent::EventType::RAW,            
                EventManagerTestFixture::basic_no_adapter_callback,
                out_event_id), 
            MblError::None);
        TR_DEBUG_POINT;  
    }
    //not enter the loop and start dispatching events.
    ASSERT_EQ(sd_event_loop(event_loop_handle_), MblError::None);

    //deinitialize event manager
    event_manager_.deinit();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////// DBusAdapeter ////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/**
 * @brief Class fixture for all DBusAdapeter tests
 * 
 */
class DBusAdapeterTestFixture : public ::testing::Test {
public:
    DBusAdapeterTestFixture() : 
        adapter_(ccrb_),
        tester_(adapter_)
        {};
    
    static int validate_service_exist(AppThread *app_thread, void *user_data);
    static void* mbl_cloud_client_thread(void *adapter);
    static int event_loop_request_stop(sd_event_source *s, void *userdata);
    static sem_t semaphore_; 

    ResourceBroker ccrb_;
    DBusAdapter adapter_;
    TestInfra_DBusAdapterTester tester_;
};
sem_t DBusAdapeterTestFixture::semaphore_;


/**
 * @brief init / deinit DBusAdapeter 10 times
 * tester TestInfra_DBusAdapterTester is used in order to access private members
 */
TEST_F(DBusAdapeterTestFixture, init_deinit)
{
    GTEST_LOG_START_TEST;    
    
    for (auto i = 0; i < 10; ++i)
    {
        ASSERT_EQ(adapter_.init(), MblError::None);
        ASSERT_EQ(adapter_.deinit(), MblError::None);
        ASSERT_EQ(tester_.validate_deinitialized_adapter(), MblError::None);
    }
}

int DBusAdapeterTestFixture::DBusAdapeterTestFixture::event_loop_request_stop(
        sd_event_source *s, void *userdata)
{
    UNUSED(s);
    TR_DEBUG("Enter");
    assert(userdata);
    TestInfra_DBusAdapterTester *tester = static_cast<TestInfra_DBusAdapterTester*>(userdata);
    return tester->event_loop_request_stop(MblError::None);
}

/**
 * @brief Construct a new TEST object
 * After initializing adapter, send a defer event and enter the event loop.
 * On callback send a request to stop with success status success. then deinit adapter.
 * This repeats 10 times. 
 * 
 */
TEST_F(DBusAdapeterTestFixture, run_stop_with_self_request)
{
    GTEST_LOG_START_TEST;
    MblError stop_status = MblError::Unknown;
    
    for (auto i = 0; i < 10; i++){        
        ASSERT_EQ(adapter_.init(), MblError::None);
        ASSERT_GE(tester_.send_event_defer(event_loop_request_stop, &tester_), 0);
        EXPECT_EQ(tester_.event_loop_run(stop_status, MblError::None), MblError::None);        
        ASSERT_EQ(adapter_.deinit(), MblError::None);
    }    
}

void* DBusAdapeterTestFixture::mbl_cloud_client_thread(void *adapter_)
{
    TR_DEBUG("Enter");
    assert(adapter_);
    DBusAdapter *adapter = static_cast<DBusAdapter *>(adapter_);
    MblError status = MblError::Unknown;;
    MblError stop_status = MblError::Unknown;;

    status = adapter->init();
    if (status != MblError::None)
    {
        pthread_exit((void *)status);
    }

    //mark ready to father thread
    if (sem_post(&semaphore_) != 0)
    {
        pthread_exit((void *)-1005);
    }

    //start run - enter loop, wait for exit request
    status = adapter->run(stop_status);
    if (status != MblError::None)
    {
        pthread_exit((void *)status);
    }
    if (stop_status != MblError::None)
    {
        pthread_exit((void *)MblError::DBA_InvalidValue);
    }

    //deinit and send success
    status = adapter->deinit();
    if (status != MblError::None)
    {
        pthread_exit((void *)status);
    }

    pthread_exit((void *)0);
}

/**
 * @brief Test that stop message can be sent via mailbox from another thread.
 * To synchronize threads, a semaphore is used. 
 * The test body thread - acts as the external thread - creates child thread (CCRB thread) , then
 * send stop request to child thread. This is done only after semaphore has been incremented to be 
 * sure that adapter has been initialized.
 * The child thread initialize the adapter, signals father thread using semaphore and enters the
 * loop. Then it deinit the adapter and send success.
 */ 

TEST_F(DBusAdapeterTestFixture, run_stop_with_external_exit_msg)
{
    GTEST_LOG_START_TEST;
    ResourceBroker ccrb;    
    pthread_t tid;
    void *retval;
    
    // i'm going to wait on a semaphore - create it , init on 0 to block
    ASSERT_EQ(sem_init(&semaphore_, 0, 0), 0);

    //start stop 10 times
    for (auto i = 0; i < 10; i++)
    {
        //create child thread and wait for signal (I am simulating mbl-cloud-client extenral thread)
        ASSERT_EQ(pthread_create(&tid, NULL, mbl_cloud_client_thread, &adapter_), 0);
        ASSERT_EQ(sem_wait(&semaphore_), 0);
        
        // child is ready - request stop and join it.
        ASSERT_EQ(adapter_.stop(MblError::None), MblError::None);        
        ASSERT_EQ(pthread_join(tid, &retval), 0);

        //check success status
        ASSERT_EQ((uintptr_t)retval, MblError::None);
    }
    
    ASSERT_EQ(sem_destroy(&semaphore_), 0);
}


int DBusAdapeterTestFixture::validate_service_exist(AppThread *app_thread, void *user_data)
{
    TR_DEBUG("Enter");
    UNUSED(user_data);
    assert(app_thread);
    AppThread *app_thread_ = static_cast<AppThread *>(app_thread);
    return app_thread_->bus_equest_name(DBUS_CLOUD_SERVICE_NAME);
}

/**
 * @brief Construct a new TEST object
 * start an AppThread to simulate a client application. Application try to request same name as the 
 * adapter service, and should fail. That's validate that name already exist on the bus.
 */
TEST_F(DBusAdapeterTestFixture, validate_service_exist)
{
    GTEST_LOG_START_TEST;
    AppThread app_thread(DBusAdapeterTestFixture::validate_service_exist, nullptr);
    void *retval = nullptr;

    ASSERT_EQ(adapter_.init(), MblError::None);        
    ASSERT_EQ(app_thread.create(), 0);
    ASSERT_EQ(app_thread.join(&retval), 0);
    ASSERT_EQ((uintptr_t)retval, -EEXIST);
    ASSERT_EQ(adapter_.deinit(), MblError::None);
}

/**
 * @brief Fixture for test DBusAdapterWithSelfEventTestFixture/adapter_self_event_callback
 * generates a set of random numbers and golds the callback count (number of times a self event
 * has dispatch) that should eventually be equal NUM_ITERATIONS
 */
class DBusAdapterWithSelfEventTestFixture : public ::testing::Test {
public:
    static const size_t NUM_ITERATIONS = 100;
    
    static MblError adapter_self_event_callback(sd_event_source *s, const SelfEvent *ev);

    void SetUp() override 
    {
        TR_DEBUG("Enter");        
        random_numbers_.empty();

        // Fill up the set - no duplicates can be in a set
        while (random_numbers_.size() < NUM_ITERATIONS){
            random_numbers_.insert(rand());
        }

        callback_count_ = 1;       
    }
    static std::set< int > random_numbers_;
    static unsigned long callback_count_;   
};
std::set< int > DBusAdapterWithSelfEventTestFixture::random_numbers_;
unsigned long DBusAdapterWithSelfEventTestFixture::callback_count_ = 1;

MblError  DBusAdapterWithSelfEventTestFixture::adapter_self_event_callback(
    sd_event_source *s, const SelfEvent *ev)
{    
    TR_DEBUG("Enter");
    const SelfEvent::EventDataType &event_data = ev->get_data();
    MblError result = MblError::None;       
    int n;

    //get the event loop handle
    sd_event* event_loop_handle = sd_event_source_get_event(s);
    if (NULL == event_loop_handle){
        sd_event_exit(sd_event_source_get_event(s), (int)MblError::DBA_SdEventCallFailure);
        return MblError::DBA_SdEventCallFailure; 
    }

    //every time a single integer is sent, find it in set and remove
    memcpy(&n, event_data.raw.bytes, sizeof(n));
    auto iter = random_numbers_.find(n);
    if (iter == random_numbers_.end()){
        sd_event_exit(event_loop_handle, (int)MblError::DBA_InvalidValue);
        return MblError::DBA_InvalidValue; 
    }
    random_numbers_.erase(iter);

    //if already called NUM_ITERATIONS times, check that set is empty and then send an exit request
    if (callback_count_ == DBusAdapterWithSelfEventTestFixture::NUM_ITERATIONS){
        if (DBusAdapterWithSelfEventTestFixture::random_numbers_.size() > 0){
            return MblError::DBA_InvalidValue;
        }
        int r = sd_event_exit(event_loop_handle, (int)result);
        if (r < 0){
            if (result != MblError::None){
                return MblError::DBA_InvalidValue;      
            }    
        }
    }
    ++callback_count_;

    return result;
}


/**
 * @brief The test send NUM_ITERATIONS times random numbers using a self event (RAW message type)
 * the exact numbers are validated on callback and the set is expected to be empty when all
 * callbacks have been handled.
 */
TEST_F(DBusAdapterWithSelfEventTestFixture, adapter_self_event_callback)
{
    GTEST_LOG_START_TEST;
    SelfEvent::EventDataType event_data = { 0 };
    uint64_t out_event_id = 0;
    MblError stop_status;
    ResourceBroker ccrb;
    DBusAdapter adapter(ccrb);
    TestInfra_DBusAdapterTester tester(adapter);

    //initialize adapter
    ASSERT_EQ(adapter.init(), MblError::None);

    // send SelfEventTest::NUM_ITERATIONS events with random non-repeating integers,
    // then start the loop
    for (auto &n : random_numbers_){
        memcpy(event_data.raw.bytes, &n, sizeof(n));
        tester.send_event_immediate(
            event_data,
            sizeof(n),
            SelfEvent::EventType::RAW,            
            DBusAdapterWithSelfEventTestFixture::adapter_self_event_callback,
            out_event_id); 
    }

    //now run and dispatch all self events
    ASSERT_EQ(adapter.run(stop_status), MblError::None);

    //deinitialize adapter
    ASSERT_EQ(adapter.deinit(), MblError::None);
}
