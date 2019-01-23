#include <gtest/gtest.h>
#include "MblSdbusBinder.h"

TEST(SquareRootTest, PositiveNos) { 
    printf("Starting...\n");
    mbl::MblSdbusBinder binder; 
    ASSERT_EQ(binder.init(), mbl::MblError::None);
}
 
extern "C" int start_service();


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();    
}