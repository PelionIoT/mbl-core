/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MblError_h_
#define MblError_h_

//#include "mbed-cloud-client/MbedCloudClient.h"

// FIXME : remove later
#define tr_info(a,b)
#define tr_debug(a,b)
#define tr_error(a,b)

namespace mbl {

namespace Error {
enum Type {
    None                                = 0x0000,
    Unknown                             = 0x0001,
    LogInitFopen                        = 0x0002,
    LogInitMutexCreate                  = 0x0003,
    SignalsInitSigaction                = 0x0004,
    DeviceUnregistered                  = 0x0005,
    ShutdownRequested                   = 0x0006,

    ConnectAlreadyExists                = 0x0100,
    ConnectBootstrapFailed              = 0x0101,
    ConnectInvalidParameters            = 0x0102,
    ConnectNotRegistered                = 0x0103,
    ConnectTimeout                      = 0x0104,
    ConnectNetworkError                 = 0x0105,
    ConnectResponseParseFailed          = 0x0106,
    ConnectUnknownError                 = 0x0107,
    ConnectMemoryConnectFail            = 0x0108,
    ConnectNotAllowed                   = 0x0109,
    ConnectSecureConnectionFailed       = 0x010a,
    ConnectDnsResolvingFailed           = 0x010b,
    ConnectorFailedToReadCredentials    = 0x010c,
    ConnectorInvalidCredentials         = 0x010d,
    ConnectorFailedToStoreCredentials   = 0x010e,

    UpdateWarningCertificateNotFound    = 0x0200,
    UpdateWarningIdentityNotFound       = 0x0201,
    UpdateWarningCertificateInvalid     = 0x0202,
    UpdateWarningSignatureInvalid       = 0x0203,
    UpdateWarningVendorMismatch         = 0x0204,
    UpdateWarningClassMismatch          = 0x0205,
    UpdateWarningDeviceMismatch         = 0x0206,
    UpdateWarningURINotFound            = 0x0207,
    UpdateWarningRollbackProtection     = 0x0208,
    UpdateWarningUnknown                = 0x0209,
    UpdateErrorWriteToStorage           = 0x020a,
    UpdateWarningNoActionRequired       = 0x020b,
    UpdateErrorUserActionRequired       = 0x020c,
    UpdateFatalRebootRequired           = 0x020d,
    UpdateErrorInvalidHash              = 0x020e,

    // TODO - replace
    AlreadyInitialized                  = 0x0300,
    NotInitialized                      = 0x0301,
    DBusErr_Temporary                   = 0x0302,

/*
    This Error is used in low level sd-event loop
    In order not to conflict with Linux error codes, start it from 0x3E0
    For more details :
    http://www-numi.fnal.gov/offline_software/srt_public_context/WebDocs/Errors/unix_system_errors.html
    https://www.freedesktop.org/software/systemd/man/sd_event_loop.html#
    https://www.freedesktop.org/software/systemd/man/sd_event_exit.html#
*/

    DBusStopStatusErrorInternal         = 0x03E1, //temporary - give better codes

    EnrollmentErrorBase                 = 0x0400,
    EnrollmentErrorEnd                  = 0x0401

};
} // namespace Error

typedef Error::Type MblError;

const char* MblError_to_str(MblError error);

//MblError CloudClientError_to_MblError(MbedCloudClient::Error error);

} // namespace mbl

#endif // MblError_h_
