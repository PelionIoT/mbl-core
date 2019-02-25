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
        RETURN_STRINGIFIED_VALUE(STATUS_SUCCESS);
        RETURN_STRINGIFIED_VALUE(ERR_FAILED);
        RETURN_STRINGIFIED_VALUE(ERR_INTERNAL_ERROR);

    default: return "Unknown CloudConnectStatus value";
    }
}

const char* CloudConnectStatus_to_readable_str(const CloudConnectStatus status)
{
    switch (status)
    {
    case STATUS_SUCCESS: return "Status success";

    case ERR_FAILED: return "General failure";

    case ERR_INTERNAL_ERROR: return "Internal error in Cloud Connect infrastructure";

    default: return "Unknown Cloud Connect Status or Error";
    }
}

const char* CloudConnectStatus_error_to_DBus_str(const CloudConnectStatus status)
{
    switch (status)
    {
        RETURN_DBUS_FORMAT_ERROR(ERR_FAILED);
        RETURN_DBUS_FORMAT_ERROR(ERR_INTERNAL_ERROR);

    default: return "mbed.Cloud.Connect.UnknownError";
    }
}

const char* ResourceDataType_stringify(const ResourceDataType type)
{
    switch (type)
    {
        RETURN_STRINGIFIED_VALUE(STRING);
        RETURN_STRINGIFIED_VALUE(INTEGER);
        RETURN_STRINGIFIED_VALUE(FLOAT);
        RETURN_STRINGIFIED_VALUE(BOOLEAN);
        RETURN_STRINGIFIED_VALUE(OPAQUE);
        RETURN_STRINGIFIED_VALUE(TIME);
        RETURN_STRINGIFIED_VALUE(OBJLINK);

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
