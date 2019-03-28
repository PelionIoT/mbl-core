/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "DBusMessagesFactory.h"
#include "CloudConnectTrace.h"
#include "DBusCloudConnectNames.h"
#include "DBusMessage.h"

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

std::shared_ptr<DBusCommonMessageProcessor>
DBusMessagesFactory::get_message_processor(const std::string method_name)
{
    TR_DEBUG_ENTER;

    auto it = message_processors_.find(method_name);
    if (it == message_processors_.end()) {

        TR_ERR("Failed to find message processor for message=%s", method_name.c_str());
        return nullptr;
    }
    assert(it->second != nullptr);
    return it->second;
}

} // namespace mbl
