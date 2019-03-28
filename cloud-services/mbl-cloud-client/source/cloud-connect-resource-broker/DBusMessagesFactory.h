/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DBusMessagesFactory_h_
#define _DBusMessagesFactory_h_

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
     * @brief Static method: find sd-bus message processor which corresponds to the sd-bus message
     * method name.
     * @param method_name - sd-bus method name
     * @return std::shared_ptr<DBusCommonMessageProcessor> - sd-bus method message processor
     */
    static std::shared_ptr<DBusCommonMessageProcessor>
    get_message_processor(const std::string method_name);

    typedef std::shared_ptr<DBusCommonMessageProcessor> DBusMsgProcessor;

private:
    // map of sd-bus method name and correspondent to this method message processor
    static std::map<std::string, DBusMsgProcessor> message_processors_;
};

} // namespace mbl

#endif //_DBusMessagesFactory_h_
