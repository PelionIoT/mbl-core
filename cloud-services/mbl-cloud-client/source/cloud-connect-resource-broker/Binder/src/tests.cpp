#include <gtest/gtest.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <semaphore.h>

#include "DBusAdapter.h"
#include "DBusAdapterMsg.h"

extern "C" {
#include "DBusAdapterMailbox.h"
}

#define DBUS_MAILBOX_WAIT_TIME_MS      100000

TEST(DBusAdapterMailBox, AllocFree) {
    DBusAdapterMailbox mailbox;

    ASSERT_EQ(DBusAdapterMailbox_alloc(&mailbox), 0);
    ASSERT_EQ(DBusAdapterMailbox_free(&mailbox), 0);
}

TEST(DBusAdapterMailBox, SendReceiveRawMessagePtr_SingleThread) {
    DBusAdapterMailbox mailbox;
    struct DBusAdapterMsg input_msg;
    struct DBusAdapterMsg output_msg;
    uintptr_t msg_ptr_as_int;
    std::string str("Hello1 Hello2 Hello3");

    memset(&input_msg, 0, sizeof(input_msg));
    input_msg.type = DBUS_ADAPTER_MSG_RAW;
    input_msg.payload_len = str.length();
    strncpy(
        input_msg.payload.raw.bytes,
        str.c_str(),
        str.length());

    ASSERT_EQ(DBusAdapterMailbox_alloc(&mailbox), 0);

    // send / receive / compare 100 times
    for (auto i = 0; i < 100; i++){
        ASSERT_EQ(DBusAdapterMailbox_send(&mailbox, &input_msg, DBUS_MAILBOX_WAIT_TIME_MS), 0);
        ASSERT_EQ(DBusAdapterMailbox_receive(&mailbox, &output_msg, DBUS_MAILBOX_WAIT_TIME_MS), 0);
        ASSERT_EQ(memcmp(&input_msg, &output_msg, sizeof(DBusAdapterMsg)), 0);
    }
    
    ASSERT_EQ(DBusAdapterMailbox_free(&mailbox), 0);
}


static void* reader_thread_start(void *mailbox)
{
    int     result;
    char    ch='A';
    DBusAdapterMsg output_msg;
    DBusAdapterMailbox *mailbox_ = (DBusAdapterMailbox*)mailbox;
    uint64_t expected_sequence_num = 0;

    for (char ch='A'; ch < 'Z'+1; ++ch, ++expected_sequence_num){
        if (DBusAdapterMailbox_receive(mailbox_, &output_msg, DBUS_MAILBOX_WAIT_TIME_MS) != 0){
            pthread_exit((void*)-1);
        }
        if ((output_msg.type != DBUS_ADAPTER_MSG_RAW) ||
            (output_msg.payload_len != 1) ||
            (output_msg.payload.raw.bytes[0] != ch)||
            (expected_sequence_num != output_msg._sequence_num))
        {
            pthread_exit((void*)-1);
        }
    }
    pthread_exit((void*)0);
}

static void* writer_thread_start(void *mailbox)
{
    char    ch='A';
    DBusAdapterMsg input_message;
    DBusAdapterMailbox *mailbox_ = (DBusAdapterMailbox*)mailbox_;
    
    memset(&input_message, 0, sizeof(input_message));
    input_message.type = DBUS_ADAPTER_MSG_RAW;
    input_message.payload_len = 1;

    for (char ch='A'; ch < 'Z'+1; ++ch){
        input_message.payload.raw.bytes[0] = ch;
        if (DBusAdapterMailbox_send(mailbox_, &input_message, DBUS_MAILBOX_WAIT_TIME_MS) != 0){
            pthread_exit((void*)-1);
        }
    }

    pthread_exit((void*)0);
}


TEST(DBusAdapterMailBox, SendReceiveRawMessagePtr_MultiThread) {
    pthread_t   writer_tid, reader_tid;
    DBusAdapterMailbox mailbox;    
    void *retval;

    ASSERT_EQ(DBusAdapterMailbox_alloc(&mailbox), 0);    
    ASSERT_EQ(pthread_create(&writer_tid, NULL, reader_thread_start, &mailbox), 0);
    ASSERT_EQ(pthread_create(&reader_tid, NULL, writer_thread_start, &mailbox), 0);
    ASSERT_EQ(pthread_join(writer_tid, &retval), 0);
    ASSERT_EQ((uintptr_t)retval, 0);
    ASSERT_EQ(pthread_join(reader_tid, &retval), 0);
    ASSERT_EQ((uintptr_t)retval, 0);
    ASSERT_EQ(DBusAdapterMailbox_free(&mailbox), 0);
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
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();    
}