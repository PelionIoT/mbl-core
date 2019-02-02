#include <gtest/gtest.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <semaphore.h>

#include "DBusAdapter.h"
#include "DBusAdapterMailbox.h"
#include "MblError.h"
#include "DBusAdapterMsg.h"
#include "DBusAdapterLowLevel_internal.h"
#include "DBusAdapterLowLevel.h"


#define DBUS_MAILBOX_WAIT_TIME_MS      100000

using MblError = mbl::MblError;

TEST(DBusAdapterMailBox_a,InitDeinit) 
{
    mbl::DBusAdapterMailbox mailbox;

    ASSERT_EQ(mailbox.init(), MblError::None);
    ASSERT_EQ(mailbox.deinit(), MblError::None);
}

TEST(DBusAdapterMailBox_a,SendReceiveRawMessagePtr_SingleThread) 
{
    mbl::DBusAdapterMailbox mailbox;
    mbl::DBusAdapterMsg write_msg;
    mbl::DBusAdapterMsg read_msg;
    uintptr_t msg_ptr_as_int;
    std::string str("Hello1 Hello2 Hello3");

    memset(&write_msg, 0, sizeof(write_msg));
    memset(&read_msg, 0, sizeof(read_msg));
    write_msg.header.sequence_num = mbl::DBusAdapterMsgType::DBUS_ADAPTER_MSG_RAW;
    read_msg.header.payload_len = str.length();
    strncpy(
        write_msg.payload.raw.bytes,
        str.c_str(),
        str.length());

    ASSERT_EQ(mailbox.init(), 0);

    // send / receive / compare 100 times
    for (auto i = 0; i < 100; i++){
        ASSERT_EQ(mailbox.send_msg(write_msg, DBUS_MAILBOX_WAIT_TIME_MS), 0);
        ASSERT_EQ(mailbox.receive_msg(read_msg, DBUS_MAILBOX_WAIT_TIME_MS), 0);
        write_msg = read_msg;
        ASSERT_EQ(memcmp(&write_msg, &read_msg, sizeof(mbl::DBusAdapterMsg)), 0);        
    }
    
    ASSERT_EQ(mailbox.deinit(), 0);
}


static void* reader_thread_start(void *mailbox)
{
    mbl::DBusAdapterMailbox* mailbox_ = static_cast<mbl::DBusAdapterMailbox*>(mailbox);
    mbl::DBusAdapterMsg output_msg;
    uint64_t expected_sequence_num = 0;

    for (char ch='A'; ch < 'Z'+1; ++ch, ++expected_sequence_num){
        if (mailbox_->receive_msg(output_msg, DBUS_MAILBOX_WAIT_TIME_MS) != 0){
            pthread_exit((void*)-1);
        }
        if ((output_msg.header.type != mbl::DBUS_ADAPTER_MSG_RAW) ||
            (output_msg.header.payload_len != 1) ||
            (output_msg.payload.raw.bytes[0] != ch)||
            (output_msg.header.sequence_num != expected_sequence_num))
        {
            pthread_exit((void*)-1);
        }
    }
    pthread_exit((void*)0);
}

static void* writer_thread_start(void *mailbox)
{
    mbl::DBusAdapterMsg input_message;
    mbl::DBusAdapterMailbox* mailbox_ = static_cast<mbl::DBusAdapterMailbox*>(mailbox);
    
    memset(&input_message, 0, sizeof(input_message));
    input_message.header.type = mbl::DBUS_ADAPTER_MSG_RAW;
    input_message.header.payload_len = 1;

    for (char ch='A'; ch < 'Z'+1; ++ch){
        input_message.payload.raw.bytes[0] = ch;
        if (mailbox_->send_msg(input_message, DBUS_MAILBOX_WAIT_TIME_MS) != 0){
            pthread_exit((void*)-1);
        }
    }

    pthread_exit((void*)0);
}


TEST(DBusAdapterMailBox_a,SendReceiveRawMessage_MultiThread) 
{
    pthread_t   writer_tid, reader_tid;
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


extern DBusAdapterLowLevelContext* DBusAdapterLowLevel_GetContext();

TEST(DBusAdapeterLowLevel_b,init_deinit) 
{
    DBusAdapterCallbacks  callbacks;

    //This is a fast dummy initialization for testing 
    callbacks.register_resources_async_callback = (int (*)(uintptr_t, const char*))1;
    callbacks.deregister_resources_async_callback = (int (*)(uintptr_t, const char*))2;

    ASSERT_EQ(DBusAdapterLowLevel_init(&callbacks), 0);
    ASSERT_EQ(DBusAdapterLowLevel_deinit(), 0);
}

TEST(DBusAdapeterLowLevel_b,run_stop) 
{
    //TODO
    //ASSERT_ANY_THROW(0);
}

TEST(DBusAdapeter_c,init_deinit) 
{
    mbl::DBusAdapter adapter;

    ASSERT_EQ(adapter.init(), 0);
    ASSERT_EQ(adapter.deinit(), 0);
}

TEST(DBusAdapeter_c, run_stop) 
{
    //TODO
    //ASSERT_ANY_THROW(0);
}

/*
typedef struct ccrm_intra_thread
{
   sem_t sem; 
   mbl::MblSdbusBinder binder;
}ccrm_intra_thread;

static void* ccrm_thread_start(void* _data)
{
    ccrm_thread_data *data = (ccrm_thread_data*)_data;    
    uintptr_t exit_code = 0;

    if (binder.init() != mbl::MblError::None){
        exit_code = -1;
    }
    else if (sem_post(&data->sem) != 0){
         exit_code = -2;
    }
    else if (binder.start() != mbl::MblError::None) {
         exit_code = -3;
    }
    
    binder.deinit();
    pthread_exit((void*)exit_code);
}


//FIXME: remove asserts whenever need to clean finalize child thread
TEST(Sdbus, StartStopWithPipeMsg) { 
    pthread_t   tid;
    int *retval;
    DBusAdapterMsg msg;
    ccrm_intra_thread data;

    memset(&msg, 0, sizeof(msg)); 
    ASSERT_EQ(data.binder.init(), mbl::MblError::None);
       
    // Lets simulate : current thread is mbl-cloud-client thread 
    // which creates ccrm thread in the next line
    ASSERT_EQ(sem_init(&data.sem, 0, 0), 0);  //init to 0 to wait for created thread signal    
    ASSERT_EQ(pthread_create(&tid, NULL, ccrm_thread_start, &data), 0); 
    ASSERT_EQ(sem_wait(&data.sem), 0);  //wait for server to start
            
    //send mesage in pipe in raw way, not via API, no need to fill data - just signal exit
    msg.type = PIPE_MSG_TYPE_EXIT;
    ASSERT_EQ(DBusAdapterMailbox_msg_send(&data.pipe, &msg), 0);

    // FIXME: all pthread join must be converted to pthread_timedjoin_np    
    ASSERT_EQ(pthread_join(tid, (void**)&retval), 0); 
    ASSERT_EQ((uintptr_t)retval, 0);
    ASSERT_EQ(sem_destroy(&data.sem), 0);
    ASSERT_EQ(DBusAdapterMailbox_destroy(&data.pipe), 0);
    pthread_exit((void*)0);
}
*/
int main(int argc, char **argv) 
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();    
}