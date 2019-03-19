/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DBusMessagesFactory_h_
#define _DBusMessagesFactory_h_

#include "CloudConnectTypes.h"

#include <systemd/sd-bus.h>

#include <map>
#include <memory>
#include <string>

namespace mbl
{

class DBusCommonMessageProcessor;
class ResourceBroker;

/**
 * @brief This class hold a map of all kinds of sd-bus message processors. When D-Bus Adapter
 * receives sd-bus message it calls process_message() API. Message processing is done by one of
 * the classes inheriting from a pure virtual DBusMsgProcessor base class.
 */
class DBusMessagesFactory
{
public:
    /**
     * @brief Static methos: find sd-bus message processor which corresponds to the sd-bus message
     * method name and call its process_message() API.
     * @param connection_handle - sd-bus bus connection handle
     * @param m - message received on sd-bus bus
     * @param ccrb - reference to the Cloud Connect resource broker
     * @param ret_error - The sd_bus_error structure carries information about a D-Bus error
     * condition.
     * @return int - 0 on success, On failure, return a negative Linux errno-style error code.
     */
    static int process_message(sd_bus* connection_handle,
                               sd_bus_message* m,
                               ResourceBroker& ccrb,
                               sd_bus_error* ret_error);

    typedef std::shared_ptr<DBusCommonMessageProcessor> DBusMsgProcessor;

private:
    // map of sd-bus method name and correspondent to this method message processor
    static std::map<std::string, DBusMsgProcessor> message_processors_;
};

} // namespace mbl

#endif //_DBusMessagesFactory_h_
