/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>

#include <systemd/sd-bus.h>

#include "CloudConnectCommon_Internal.h"
#include "TestInfraAppThread.h"


AppThread::AppThread(std::function<int(AppThread*, void *)> user_callback, void *user_data) : 
        user_callback_(user_callback), user_data_(user_data)
{
    tr_debug("Enter");
    assert(user_callback);
}

int AppThread::create()
{
    tr_debug("Enter");
    return pthread_create(&tid_, NULL, thread_main, this);
}

int AppThread::join(void **retval)
{
    tr_debug("Enter");
    return pthread_join(tid_, retval);
}

int AppThread::start()
{
    tr_debug("Enter");
    int r = sd_bus_open_user(&connection_handle_);
    if (r < 0)
    {
        pthread_exit((void *)-1000);
    }
    if (nullptr == connection_handle_)
    {
        pthread_exit((void *)-1001);
    }

    // This part must be the last
    int ret_val = user_callback_(this, user_data_);
    sd_bus_unref(connection_handle_);
    return ret_val;
}

void *AppThread::thread_main(void *app_thread)
{
    tr_debug("Enter");
    assert(app_thread);
    AppThread *app_thread_ = static_cast<AppThread *>(app_thread);
    pthread_exit((void *)(uintptr_t)app_thread_->start());
}

