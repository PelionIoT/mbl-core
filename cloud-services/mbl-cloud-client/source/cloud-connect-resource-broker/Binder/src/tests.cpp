#include <gtest/gtest.h>
#include "MblSdbusBinder.h"
 
TEST(SquareRootTest, PositiveNos) { 
    ASSERT_EQ(1, 1);
}
 
extern "C" int start_service();


int main(int argc, char **argv) {
    //testing::InitGoogleTest(&argc, argv);
    //return RUN_ALL_TESTS();

    printf("Starting...\n");
    mbl::MblSdbusBinder binder; 
    binder.Init();   

    //start_service();    
}