/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TestsInfra_AppThread_h_
#define _TestsInfra_AppThread_h_

#include <functional>
#include <pthread.h>

struct sd_bus;

class AppThread
{
  public:
    AppThread(std::function<int(AppThread*, void *)> user_callback, void *user_data);

    int create();
    int join(void **retval);
    sd_bus *connection_handle_;

  private:
    int start();   
    static void *thread_main(void *app_thread);
    std::function<int(AppThread*,void *)> user_callback_;
    void *user_data_;
    pthread_t tid_;
};

#endif //#ifndef _TestsInfra_AppThread_h_