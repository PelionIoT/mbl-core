/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "DBusMessagesFactory.h"
#include "CloudConnectTrace.h"
#include "DBusCloudConnectNames.h"
#include "DBusMessage.h"

#include <sstream>

#include <systemd/sd-bus.h>

#define TRACE_GROUP "ccrb-dbus"

namespace mbl
{

std::map<std::string, DBusMessagesFactory::DBusMsgProcessor>
    DBusMessagesFactory::message_processors_ = {

        std::make_pair(
            DBUS_CC_REGISTER_RESOURCES_METHOD_NAME,
            DBusMessagesFactory::DBusMsgProcessor(new DBusRegisterResourcesMessageProcessor())),

        std::make_pair(
            DBUS_CC_DEREGISTER_RESOURCES_METHOD_NAME,
            DBusMessagesFactory::DBusMsgProcessor(new DBusDeregisterResourcesMessageProcessor())),

        std::make_pair(
            DBUS_CC_GET_RESOURCES_VALUES_METHOD_NAME,
            DBusMessagesFactory::DBusMsgProcessor(new DBusGetResourcesMessageProcessor())),

        std::make_pair(
            DBUS_CC_SET_RESOURCES_VALUES_METHOD_NAME,
            DBusMessagesFactory::DBusMsgProcessor(new DBusSetResourcesMessageProcessor()))};

int DBusMessagesFactory::process_message(sd_bus* connection_handle,
                                         sd_bus_message* m,
                                         ResourceBroker& ccrb,
                                         sd_bus_error* ret_error)
{
    TR_DEBUG_ENTER;
    assert(connection_handle);
    assert(m);
    assert(ret_error);

    std::string method_name = sd_bus_message_get_member(m);

    auto it = message_processors_.find(method_name);
    if (it == message_processors_.end()) {

        // TODO: - probably need to reply with error reply to sender?

        TR_ERR("Failed to find message processor for message=%s", method_name.c_str());
        return (-EBADMSG);
    }
    if (nullptr == it->second) {
        TR_ERR("Failed to find message processor for message=%s", method_name.c_str());
        return (-EBADMSG);
    }

    TR_INFO("Starting to process %s method call from sender %s",
            it->first.c_str(),
            sd_bus_message_get_sender(m));

    // process the message
    int r = it->second->process_message(connection_handle, m, ccrb, ret_error);
    if (r < 0) {
        TR_ERR("process_message failed, r=%d", r);
        return r;
    }

    return 0;
}

} // namespace mbl {
