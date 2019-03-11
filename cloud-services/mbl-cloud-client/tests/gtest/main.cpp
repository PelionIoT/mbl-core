/*
* Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include "log.h"
#include "CloudConnectTrace.h"
#include "MblError.h"

#include <gtest/gtest.h>

#include <stdlib.h>

#define TRACE_GROUP                             "ccrb-gtest"

#define MBL_CLOUD_CLIENT_GTEST_LOG_PATH         "/var/tmp/mbl-cloud-client.log"
#define MBL_CLOUD_CLIENT_DAEMON_NAME            "mbl-cloud-client"
#define MBL_CLOUD_CLIENT_DAEMON_INIT_FILE_PATH  "/etc/init.d/mbl-cloud-client"
#define MBL_CLOUD_CLIENT_INIT_SCRIPT_START_CMD  MBL_CLOUD_CLIENT_DAEMON_INIT_FILE_PATH " start"  
#define MBL_CLOUD_CLIENT_INIT_SCRIPT_STOP_CMD   MBL_CLOUD_CLIENT_DAEMON_INIT_FILE_PATH " stop"

/**
 * @brief initialize test environment :
 * 1) Stop mbl-cloud-client daemon
 * 
 * @return 0 on success, or Linux style negative error on failure 
 */
static int mbl_cloud_client_gtest_init();

/**
 * @brief deinitialize test environment :
 * 1) Start the mbl-cloud-client daemon
 * 
 * @return 0 on success, or Linux style negative error on failure 
 */
static int mbl_cloud_client_gtest_deinit();

int main(int argc, char **argv) 
{    
    TR_DEBUG_ENTER;

    // == must be first ==
    // Initialize/config test environment for the test for mbl
    int ret1 = mbl_cloud_client_gtest_init();
    if (ret1 < 0){
        TR_ERR("mbl_cloud_client_gtest_init() failed with error=%s (r=%d)", strerror(ret1), ret1);
        exit(ret1);
    }
    
    // Initialize google test
    ::testing::InitGoogleTest(&argc, argv);

    // run the tests
    int ret2 = RUN_ALL_TESTS();
    TR_INFO("=== RUN_ALL_TESTS() returned status : (%d) ===", ret2);

    // == must be last ==
    // Restore the environment to the previous status before gtests was running
    ret1 = mbl_cloud_client_gtest_deinit();
    if (ret1 < 0){
        TR_ERR("mbl_cloud_client_gtest_deinit() failed with error=%s (r=%d),", strerror(ret1), ret1);
        exit(ret1);
    }

    return ret2;
}

/**
 * @brief check if file exist
 * 
 * @param path fill path to the file
 * @return true 
 * @return false 
 */
inline bool is_file_exist (const std::string& path) {
    return ( access( path.c_str(), F_OK ) != -1 );
}

/**
 * @brief Initialize gtest environment for mbl-cloud-client-gtest :
 * * Stop mbl-cloud-client if exist
 * * Initialize Cloud Client log
 * * Make sure that trace level is debug
 * 
 * @return int 
 */
static int mbl_cloud_client_gtest_init()
{
    TR_DEBUG_ENTER;
    
    // stop mbl-cloud-client if exist - this should be usually done on a target. This is done as 
    // best effort - we do not walk the /proc and do not check if process really exist.
    // The presence of an init file is enough to invoke a stop command
    if (is_file_exist(MBL_CLOUD_CLIENT_DAEMON_INIT_FILE_PATH)){
        int ret = system(MBL_CLOUD_CLIENT_INIT_SCRIPT_STOP_CMD);
        if (0 == ret){
            TR_INFO("%s stopped!", MBL_CLOUD_CLIENT_DAEMON_NAME);
        }
        else {
            TR_INFO("failed to stop %s with error %d! exiting..", 
                MBL_CLOUD_CLIENT_DAEMON_NAME, ret);
            exit(ret);
        }
    }

    // Initialize Cloud Client log
    mbl::MblError retval = mbl::log_init(MBL_CLOUD_CLIENT_GTEST_LOG_PATH);
    if(mbl::Error::None != retval){
        //if mbl::log_init failed - stop test. It doesn't make sense to test without logs    
        printf("Error init Cloud Client log: %s", mbl::MblError_to_str(retval));
        return -1;
    }

    // Make sure that trace level is debug as we want as much information posibble when running tests
    // in case error occured. This ovvirides mbl-cloud-client default trace level.
    mbed_trace_config_set(TRACE_ACTIVE_LEVEL_DEBUG);

    return 0;
}

/**
 * @brief Deinitialize gtest environment for mbl-cloud-client-gtest
 * 
 * @return int 
 */
static int mbl_cloud_client_gtest_deinit()
{   
    TR_DEBUG_ENTER;

    if (is_file_exist(MBL_CLOUD_CLIENT_DAEMON_INIT_FILE_PATH)){
        int ret = system(MBL_CLOUD_CLIENT_INIT_SCRIPT_START_CMD);
        if (0 == ret){
            TR_INFO("%s started!", MBL_CLOUD_CLIENT_DAEMON_NAME);
        }
        else {
            TR_INFO("failed to start %s with error %d!", 
                MBL_CLOUD_CLIENT_DAEMON_NAME, ret);
            //continue
            return ret;
        }
    }

    return 0;
}
