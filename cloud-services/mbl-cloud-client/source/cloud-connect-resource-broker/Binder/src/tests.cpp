#include <gtest/gtest.h>
#include <pthread.h>
#include <stdio.h>

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


void* reader_thread_start(void *_pipe)
{
    int     result;
    char    ch='A';
    MblSdbusPipeMsg *read_msg;
    MblSdbusPipe *pipe = (MblSdbusPipe*)_pipe;

    for (char ch='A'; ch < 'Z'+1; ++ch){
        if (MblSdbusPipe_message_receive(pipe, &read_msg) != 0){
            pthread_exit((void*)-1);
        }
        if ((read_msg->msg_type != PIPE_MSG_TYPE_DUMMY) ||
            (read_msg->data.dummy_msg.msg[0] != ch)) {
                 pthread_exit((void*)-1);
        }
        std::cout << ch << ":" <<  read_msg->data.dummy_msg.msg[0] << std::endl;
        free(read_msg);
    }
    pthread_exit((void*)0);
}

void* writer_thread_start(void *_pipe)
{
    int     result;
    char    ch='A';
    MblSdbusPipeMsg write_msg;
    MblSdbusPipe *pipe = (MblSdbusPipe*)_pipe;

    memset(&write_msg, 0, sizeof(write_msg));
    write_msg.msg_type = PIPE_MSG_TYPE_DUMMY;

    for (char ch='A'; ch < 'Z'+1; ++ch){
        write_msg.data.dummy_msg.msg[0] = ch;
        if (MblSdbusPipe_send_message(pipe, &write_msg) != 0){
            pthread_exit((void*)-1);
        }
    }

    pthread_exit((void*)0);
}



TEST(Sdbus, SendReceiveDummyMessageMultiThread) {
    pthread_t   tid1,tid2;
    MblSdbusPipe pipe;    
    int r, *retval;

    ASSERT_EQ(MblSdbusPipe_create(&pipe), 0);    
    ASSERT_EQ(pthread_create(&tid1, NULL, reader_thread_start, &pipe), 0);
    ASSERT_EQ(pthread_create(&tid2, NULL, writer_thread_start, &pipe), 0);
    ASSERT_EQ(pthread_join(tid1, (void**)&retval), 0);
    ASSERT_EQ(pthread_join(tid2, (void**)&retval), 0);
    ASSERT_EQ(MblSdbusPipe_destroy(&pipe), 0);
}



int ffs(int a, std::string aa="")
{
    return 0;
}
int main(int argc, char **argv) {

   
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();    
}


/**
 * @brief Mbl Cloud Connect resource data type.
 * Currently supported LwM2M resource data types. 
 */
    enum class MblCloudConnectResourceDataType {
        INVALID   = 0x0,
        STRING    = 0x1,
        INTEGER   = 0x2,
        FLOAT     = 0x3,
        BOOLEAN   = 0x4,
        OPAQUE    = 0x5,
        TIME      = 0x6,
        OBJLINK   = 0x7,
    };
    
/**
 * @brief Class that implements resource data value holder. 
 */
class MblResourceDataValue {
public:

    MblResourceDataValue();

/**
 * @brief Construct a new Mbl Resource Data Value object and 
 * stores provided string.
 * @param str data that should be stored.
 */
    MblResourceDataValue(const std::string &str);

/**
 * @brief Construct a new Mbl Resource Data Value object and 
 * stores provided integer.
 * @param integer data that should be stored.
 */
    MblResourceDataValue(int64_t integer);

/**
 * @brief Stores provided string. 
 * 
 * @param str data that should be stored.
 */
    void set_value(const std::string &str); 

/**
 * @brief Stores provided integer.
 * @param integer data that should be stored.
 */
    void set_value(int64_t integer);

/**
 * @brief Gets the value of stored string.  
 * @return std::string returned value.
 */
    std::string get_value_string();

/**
 * @brief Gets the value of stored integer.  
 * @return int64_t returned value.
 */
    int64_t get_value_integer();
    
private:
    // for current moment we use simple implementation for integer and string.
    // When we shall have more types, consider using union or byte array for 
    // storing different types. 
    std::string string_data_value_;
    int64_t integer_data_value_;

    // stores type of stored data
    MblCloudConnectResourceDataType data_type_ = MblCloudConnectResourceDataType::INVALID;

    // currently we don't have pointers in class members, so we can allow default ctor's 
    // and assign operators without worry.  
};


/**
 * @brief [resource_path, resource_typed_data_value] tuple.
 */
    struct MblCloudConnect_ResourcePath_Value
    {
        std::string path;
        MblResourceDataValue typed_data_value;
    };

/**
 * @brief [resource_path, resource_data_type] tuple.
 */
    struct MblCloudConnect_ResourcePath_Type
    {
        std::string path;
        MblCloudConnectResourceDataType data_type;
    };

/**
 * @brief [resource_path, resource_typed_data_value, operation_status] tuple.
 */
    struct MblCloudConnect_ResourcePath_Value_Status
    {
        std::string path;
        MblResourceDataValue typed_data_value;
        int operation_status;
    };

/**
 * @brief [resource_path, operation_status] tuple.
 */
    struct MblCloudConnect_ResourcePath_Status
    {
        std::string path;
        int operation_status;
    };

    struct MblCloudConnectResourceOperationData
    {   
        //operation input
        struct input {
            const std::string path;
            const MblResourceDataValue resource_data;
        }input;

        //operation output
        struct output {
            int status;
        }output;
    };


     int set_resource_values(
        const std::string access_token, 
        const std::vector<MblCloudConnectResourceOperationData> &operations_data);


    int get_resource_values(
        const std::string access_token, 
        const std::vector<MblCloudConnectResourceOperationData> &operations_data);


enum Color { red, green, blue };
Color f()
{
    return Color::blue;
}





    struct MblCloudConnectResourceOperationData
    {   
        MblCloudConnectResourceOperationData(); /// ADD constructor(s) for fast initialization
        /// multi ctors...
        // ....

        //operation input
        struct input {
            const std::string path;
            const MblResourceDataValue resource_data;
        }input;
        
        //operation output
        struct output {
            MblError status;
        }output;
    };

     MblError set_resource_values(
        const std::string access_token, 
        const std::vector<MblCloudConnectResourceOperationData> &operations_data);

    MblError get_resource_values(
        const std::string access_token, 
        const std::vector<MblCloudConnectResourceOperationData> &operations_data);