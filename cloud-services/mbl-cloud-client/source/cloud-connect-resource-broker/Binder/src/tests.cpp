#include <gtest/gtest.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <semaphore.h>

#include "MblSdbusBinder.h"


extern "C" {
#include "MblSdbusPipe.h"
}

TEST(Sdbus, CreateDestroyPipe) {
    MblSdbusPipe pipe;

    ASSERT_EQ(MblSdbusPipe_create(&pipe), 0);
    ASSERT_EQ(MblSdbusPipe_destroy(&pipe), 0);
}

TEST(Sdbus, SendReceiveRawMessageSingleThread) {
    MblSdbusPipe pipe;
    MblSdbusPipeMsg write_msg;
    MblSdbusPipeMsg *read_msg;

    write_msg.type = PIPE_MSG_TYPE_RAW;
    strncpy(write_msg.msg.raw.bytes, "Hello1 Hello2 Hello3", sizeof(write_msg.msg.raw.bytes));;

    ASSERT_EQ(MblSdbusPipe_create(&pipe), 0);
    ASSERT_EQ(MblSdbusPipe_msg_send(&pipe, &write_msg), 0);
    ASSERT_EQ(MblSdbusPipe_msg_receive(&pipe, &read_msg), 0);
    ASSERT_EQ(strcmp(write_msg.msg.raw.bytes, read_msg->msg.raw.bytes), 0);
    ASSERT_EQ(MblSdbusPipe_destroy(&pipe), 0);
    free(read_msg);
}

static void* reader_thread_start(void *_pipe)
{
    int     result;
    char    ch='A';
    MblSdbusPipeMsg *read_msg;
    MblSdbusPipe *pipe = (MblSdbusPipe*)_pipe;

    for (char ch='A'; ch < 'Z'+1; ++ch){
        if (MblSdbusPipe_msg_receive(pipe, &read_msg) != 0){
            pthread_exit((void*)-1);
        }
        if ((read_msg->type != PIPE_MSG_TYPE_RAW) ||
            (read_msg->msg.raw.bytes[0] != ch)) {
                 pthread_exit((void*)-1);
        }
        std::cout << ch << ":" <<  read_msg->msg.raw.bytes[0] << std::endl;
        free(read_msg);
    }
    pthread_exit((void*)0);
}

static void* writer_thread_start(void *_pipe)
{
    char    ch='A';
    MblSdbusPipeMsg write_msg;
    MblSdbusPipe *pipe = (MblSdbusPipe*)_pipe;

    memset(&write_msg, 0, sizeof(write_msg));
    write_msg.type = PIPE_MSG_TYPE_RAW;

    for (char ch='A'; ch < 'Z'+1; ++ch){
        write_msg.msg.raw.bytes[0] = ch;
        if (MblSdbusPipe_msg_send(pipe, &write_msg) != 0){
            pthread_exit((void*)-1);
        }
    }

    pthread_exit((void*)0);
}

TEST(Sdbus, SendReceiveRawMessageMultiThread) {
    pthread_t   tid1,tid2;
    MblSdbusPipe pipe;    
    int *retval;

    ASSERT_EQ(MblSdbusPipe_create(&pipe), 0);    
    ASSERT_EQ(pthread_create(&tid1, NULL, reader_thread_start, &pipe), 0);
    ASSERT_EQ(pthread_create(&tid2, NULL, writer_thread_start, &pipe), 0);
    ASSERT_EQ(pthread_join(tid1, (void**)&retval), 0);
    ASSERT_EQ((uintptr_t)retval, 0);
    ASSERT_EQ(pthread_join(tid2, (void**)&retval), 0);
    ASSERT_EQ((uintptr_t)retval, 0);
    ASSERT_EQ(MblSdbusPipe_destroy(&pipe), 0);
}

typedef struct ccrm_thread_data
{
   sem_t sem; 
   MblSdbusPipe pipe;
}ccrm_thread_data;

static void* ccrm_thread_start(void* _data)
{
    mbl::MblSdbusBinder binder;
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

/*
//FIXME: remove asserts whenever need to clean finalize child thread
TEST(Sdbus, StartStopWithPipeMsg) { 
    pthread_t   tid;
    int *retval;
    MblSdbusPipeMsg msg;
    ccrm_thread_data data;
    
    ASSERT_EQ(MblSdbusPipe_create(&data.pipe), 0);
    memset(&msg, 0, sizeof(msg));
    
    // Lets simulate : current thread is mbl-cloud-client thread 
    // which creates ccrm thread in the next line
    ASSERT_EQ(sem_init(&data.sem, 0, 0), 0);  //init to 0 to wait for created thread signal    
    ASSERT_EQ(pthread_create(&tid, NULL, ccrm_thread_start, &data), 0); 
    ASSERT_EQ(sem_wait(&data.sem), 0);  //wait for server to start
            
    //send mesage in pipe, no need to fill data - just signal exit
    msg.type = PIPE_MSG_TYPE_EXIT;
    ASSERT_EQ(MblSdbusPipe_msg_send(&data.pipe, &msg), 0);

    // FIXME: all pthread join must be converted to pthread_timedjoin_np    
    ASSERT_EQ(pthread_join(tid, (void**)&retval), 0); 
    ASSERT_EQ((uintptr_t)retval, 0);
    ASSERT_EQ(sem_destroy(&data.sem), 0);
    ASSERT_EQ(MblSdbusPipe_destroy(&data.pipe), 0);
    pthread_exit((void*)0);
}
*/

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();    
}