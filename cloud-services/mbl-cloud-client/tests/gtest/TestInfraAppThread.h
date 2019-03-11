/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TestInfraAppThread_h_
#define _TestInfraAppThread_h_

#include <functional>
#include <pthread.h>

struct sd_bus;

// TODO: continue develop more infra-capabilities on client side to simulate app behavior and
// re-use code in tests

/**
 * @brief This is a basic infrastructure thread class - it holds the pthread_create/join operations
 * , connects to the bus and invokes user_callback_ according to specific test
 *
 */
class AppThread
{
public:
    /**
     * @brief Construct a new App Thread object
     *
     * @param user_callback - user callback to be called after connecting to bus
     * @param user_data - user data to pass into the callback
     */
    AppThread(std::function<int(AppThread*, void*)> user_callback, void* user_data);

    /**
     * @brief creates the thread
     *
     * @return int - as in pthread_create
     */
    int create();

    /**
     * @brief  joins the thread
     *
     * @param retval - the thread return value
     * @return int - as in pthread_join
     */
    int join(void** retval);

    /**
     * @brief request a name on the bus
     *
     * @param name - name ot request
     * @return int - as in sd_bus_request_name()
     */
    int bus_request_name(const char* name);

    /**
     * @brief return d-bus connection handle
     * 
     * @return sd_bus * d-bus connection handle
     */
    sd_bus *get_connection_handle() const;

  private:
    int start();   
    static void *thread_main(void *app_thread);

    // TODO: consider convert this to a vector of std::functions to prefor multiple operations
    // The user callback to be invoked after object invoked a new thread and connected to bus
    std::function<int(AppThread*, void*)> user_callback_;

    // user data to pass with the user callback
    void* user_data_;

    // the thread ID
    pthread_t tid_;

    // The thread bus connection handle
    sd_bus* connection_handle_;
};

#endif //#ifndef _TestInfraAppThread_h_
