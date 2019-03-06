/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "CloudConnectTypes.h"
#include "CloudConnectTrace.h"
#include "MblError.h"

#include <cassert>

#define TRACE_GROUP "ccrb"

namespace mbl
{

ResourceData::ResourceData(const std::string& path, const ResourceDataType type)
    : path_(path), event_type_(type)
{
    TR_DEBUG("Enter");
}

ResourceData::ResourceData(const std::string& path, const std::string& initial_value)
    : path_(path), string_value_(initial_value), event_type_(ResourceDataType::STRING)
{
    TR_DEBUG("Enter");
}

ResourceData::ResourceData(const std::string& path, int64_t initial_value)
    : path_(path), integer_value_(initial_value), event_type_(ResourceDataType::INTEGER)
{
    TR_DEBUG("Enter");
}

std::string ResourceData::get_path() const
{
    TR_DEBUG("Enter");
    return path_;
}

ResourceDataType ResourceData::get_data_type() const
{
    TR_DEBUG("Enter");
    return event_type_;
}

void ResourceData::set_value(const std::string& value)
{
    TR_DEBUG("Enter");
    assert(event_type_ == ResourceDataType::STRING);
    string_value_ = value;
}

void ResourceData::set_value(int64_t value)
{
    TR_DEBUG("Enter");
    assert(event_type_ == ResourceDataType::INTEGER);
    integer_value_ = value;
}

std::string ResourceData::get_value_string() const
{
    TR_DEBUG("Enter");
    assert(event_type_ == ResourceDataType::STRING);
    return string_value_;
}

int64_t ResourceData::get_value_integer() const
{
    TR_DEBUG("Enter");
    assert(event_type_ == ResourceDataType::INTEGER);
    return integer_value_;
}

const char* CloudConnectStatus_to_str(const CloudConnectStatus status)
{
    switch (status)
    {
        SWITCH_RETURN_STRINGIFIED_VALUE(STATUS_SUCCESS);
        SWITCH_RETURN_STRINGIFIED_VALUE(ERR_INTERNAL_ERROR);
        SWITCH_RETURN_STRINGIFIED_VALUE(ERR_INVALID_APPLICATION_RESOURCES_DEFINITION);
        SWITCH_RETURN_STRINGIFIED_VALUE(ERR_REGISTRATION_ALREADY_IN_PROGRESS);
        SWITCH_RETURN_STRINGIFIED_VALUE(ERR_ALREADY_REGISTERED);

    default: return "Unknown CloudConnectStatus value";
    }
}

const char* CloudConnectStatus_to_readable_str(const CloudConnectStatus status)
{
    switch (status)
    {
    case STATUS_SUCCESS: return "Status success";
    case ERR_INTERNAL_ERROR: return "Internal error in Cloud Connect infrastructure";
    case ERR_INVALID_APPLICATION_RESOURCES_DEFINITION:
        return "Invalid application resource definition";
    case ERR_REGISTRATION_ALREADY_IN_PROGRESS: return "Registration already in progress";
    case ERR_ALREADY_REGISTERED: return "Already registered";
    case ERR_INVALID_ACCESS_TOKEN: return "Invalid access token";
    case ERR_INVALID_RESOURCE_PATH: return "Invalid resource path";
    case ERR_RESOURCE_NOT_FOUND: return "Resource not found";
    case ERR_INVALID_RESOURCE_TYPE: return "Invalid resource type";

    default: return "Unknown Cloud Connect Status or Error";
    }
}

const char* CloudConnectStatus_error_to_DBus_format_str(const CloudConnectStatus status)
{
    switch (status)
    {
        RETURN_DBUS_FORMAT_ERROR(ERR_INTERNAL_ERROR);
        RETURN_DBUS_FORMAT_ERROR(ERR_INVALID_APPLICATION_RESOURCES_DEFINITION);
        RETURN_DBUS_FORMAT_ERROR(ERR_REGISTRATION_ALREADY_IN_PROGRESS);
        RETURN_DBUS_FORMAT_ERROR(ERR_ALREADY_REGISTERED);
        RETURN_DBUS_FORMAT_ERROR(ERR_INVALID_ACCESS_TOKEN);
        RETURN_DBUS_FORMAT_ERROR(ERR_INVALID_RESOURCE_PATH);
        RETURN_DBUS_FORMAT_ERROR(ERR_RESOURCE_NOT_FOUND);
        RETURN_DBUS_FORMAT_ERROR(ERR_INVALID_RESOURCE_TYPE);

    default: return "Unknown CloudConnectStatus value";
    }
}

const char* ResourceDataType_to_str(const ResourceDataType type)
{
    switch (type)
    {
        SWITCH_RETURN_STRINGIFIED_VALUE(STRING);
        SWITCH_RETURN_STRINGIFIED_VALUE(INTEGER);
        SWITCH_RETURN_STRINGIFIED_VALUE(FLOAT);
        SWITCH_RETURN_STRINGIFIED_VALUE(BOOLEAN);
        SWITCH_RETURN_STRINGIFIED_VALUE(OPAQUE);
        SWITCH_RETURN_STRINGIFIED_VALUE(TIME);
        SWITCH_RETURN_STRINGIFIED_VALUE(OBJLINK);

    default: return "Unknown Resource Data Type";
    }
}

OneSetMblError::OneSetMblError() : err_(MblError::None), one_time_set_flag_(true)
{
    TR_DEBUG("Enter");
};

void OneSetMblError::set(const MblError new_val)
{
    TR_DEBUG("Enter");
    if (new_val == err_) {
        TR_DEBUG("Same value, return!");
        return;
    }
    if (!one_time_set_flag_) {
        TR_DEBUG("Set already, return!");
        return;
    }
    err_ = new_val;
    one_time_set_flag_ = false;
    TR_DEBUG("Set to new value %s", get_status_str());
}

MblError OneSetMblError::get()
{
    TR_DEBUG("Enter");
    return err_;
}

const char* OneSetMblError::get_status_str()
{
    TR_DEBUG("Enter");
    return MblError_to_str(err_);
}

} // namespace mbl {
