#include <gtest/gtest.h>
#include "MblSdbusBinder.h"

extern "C" {
#include "MblSdbusPipe.h"
}

TEST(Sdbus, dummy) { 
    printf("Starting...\n");
    mbl::MblSdbusBinder binder; 
    ASSERT_EQ(binder.init(), mbl::MblError::None);
    ASSERT_EQ(binder.start(), mbl::MblError::None);
}

TEST(Sdbus, CreateDestroyPipe) {
    MblSdbusPipe pipe;

    ASSERT_EQ(MblSdbusPipe_create(&pipe), 0);
    ASSERT_EQ(MblSdbusPipe_destroy(&pipe), 0);
}

TEST(Sdbus, SendReceiveDummyMessageSingleThread) {
    MblSdbusPipe pipe;
    MblSdbusPipeMsg write_msg;
    MblSdbusPipeMsg *read_msg;

    write_msg.msg_type = PIPE_MSG_TYPE_DUMMY;
    strncpy(write_msg.data.dummy_msg.msg, "hello hello hello hello hello", sizeof(write_msg.data.dummy_msg));

    ASSERT_EQ(MblSdbusPipe_create(&pipe), 0);
    ASSERT_EQ(MblSdbusPipe_send_message(&pipe, &write_msg), 0);
    ASSERT_EQ(MblSdbusPipe_message_receive(&pipe, &read_msg), 0);
    ASSERT_EQ(strcmp(write_msg.data.dummy_msg.msg, read_msg->data.dummy_msg.msg), 0);
    ASSERT_EQ(MblSdbusPipe_destroy(&pipe), 0);
    free(read_msg);
}

TEST(Sdbus, SendReceiveDummyMessageMultiThread) {
    MblSdbusPipe pipe;
    MblSdbusPipeMsg write_msg;
    MblSdbusPipeMsg *read_msg;

    write_msg.msg_type = PIPE_MSG_TYPE_DUMMY;
    strncpy(write_msg.data.dummy_msg.msg, "hello hello hello hello hello", sizeof(write_msg.data.dummy_msg));

    ASSERT_EQ(MblSdbusPipe_create(&pipe), 0);
    ASSERT_EQ(MblSdbusPipe_send_message(&pipe, &write_msg), 0);
    ASSERT_EQ(MblSdbusPipe_message_receive(&pipe, &read_msg), 0);
    ASSERT_EQ(strcmp(write_msg.data.dummy_msg.msg, read_msg->data.dummy_msg.msg), 0);
    ASSERT_EQ(MblSdbusPipe_destroy(&pipe), 0);
    free(read_msg);
}
extern "C" int start_service();


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();    
}