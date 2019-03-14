/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TestInfraAppThread.h"
#include "CloudConnectTrace.h"

#include <systemd/sd-bus.h>

#include <cassert>

#define TRACE_GROUP "gtest-infra"

AppThread::AppThread(std::function<int(AppThread*, void*)> user_callback, void* user_data)
    : user_callback_(user_callback), user_data_(user_data)
{
    TR_DEBUG_ENTER;
    assert(user_callback);
}

int AppThread::create()
{
    TR_DEBUG_ENTER;
    return pthread_create(&tid_, NULL, thread_main, this);
}

int AppThread::join(void** retval)
{
    TR_DEBUG_ENTER;
    return pthread_join(tid_, retval);
}

int AppThread::start()
{
    TR_DEBUG_ENTER;
    int r = sd_bus_open_user(&connection_handle_);
    if (r < 0) {
        TR_ERR("sd_bus_open_user failed with error r=%d (%s)", r, strerror(-r));
        pthread_exit((void*) -1000);
    }
    if (nullptr == connection_handle_) {
        TR_ERR("sd_bus_open_user failed connection_handle_ is nullptr!");
        pthread_exit((void*) -1001);
    }

    // Get my unique name on the bus
    const char * unique_name = nullptr;
    r = sd_bus_get_unique_name(connection_handle_, &unique_name);
    if (r < 0) {
        TR_ERR("sd_bus_get_unique_name failed with error r=%d (%s)", r, strerror(-r));
        pthread_exit((void*) -1002);
    }
    assert(unique_name);
    unique_name_.assign(unique_name);

    // This part must be the last
    int ret_val = user_callback_(this, user_data_);
    sd_bus_unref(connection_handle_);
    return ret_val;
}

void* AppThread::thread_main(void* app_thread)
{
    TR_DEBUG_ENTER;
    assert(app_thread);
    AppThread* app_thread_ = static_cast<AppThread*>(app_thread);
    pthread_exit((void*) (uintptr_t) app_thread_->start());
}

int AppThread::bus_request_name(const char* name) const
{ 
    return sd_bus_request_name(connection_handle_, name, 0);
}

void AppThread::disconnect()
{
    TR_DEBUG_ENTER;
    if (connection_handle_){
        connection_handle_ = sd_bus_flush_close_unref(connection_handle_);
    }    
}
