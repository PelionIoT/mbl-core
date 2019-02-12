/*
 * Copyright (c) 2017 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#include "MblError.h"

namespace mbl {

const char* MblError_to_str(const MblError error)
{
    switch (error) {
        case Error::None: return "None";
        case Error::Unknown: return "Unknown";
        case Error::LogInitFopen: return "fopen failed";
        case Error::LogInitMutexCreate: return "Failed to create mutex for logging";
        case Error::SignalsInitSigaction: return "Failed to register signal handler";
        case Error::DeviceUnregistered: return "Device became unregistered";
        case Error::ShutdownRequested: return "Shutdown requested";

        case Error::ConnectAlreadyExists: return "ConnectAlreadyExists";
        case Error::ConnectBootstrapFailed: return "ConnectBootstrapFailed";
        case Error::ConnectInvalidParameters: return "ConnectInvalidParameters";
        case Error::ConnectNotRegistered: return "ConnectNotRegistered";
        case Error::ConnectTimeout: return "ConnectTimeout";
        case Error::ConnectNetworkError: return "ConnectNetworkError";
        case Error::ConnectResponseParseFailed: return "ConnectResponseParseFailed";
        case Error::ConnectUnknownError: return "ConnectUnknownError";
        case Error::ConnectMemoryConnectFail: return "ConnectMemoryConnectFail";
        case Error::ConnectNotAllowed: return "ConnectNotAllowed";
        case Error::ConnectSecureConnectionFailed: return "ConnectSecureConnectionFailed";
        case Error::ConnectDnsResolvingFailed: return "ConnectDnsResolvingFailed";
        case Error::ConnectorFailedToReadCredentials: return "ConnectorFailedToReadCredentials";
        case Error::ConnectorInvalidCredentials: return "ConnectorInvalidCredentials";
        case Error::ConnectorFailedToStoreCredentials: return "ConnectorFailedToStoreCredentials";

        case Error::UpdateWarningCertificateNotFound: return "UpdateWarningCertificateNotFound";
        case Error::UpdateWarningIdentityNotFound: return "UpdateWarningIdentityNotFound";
        case Error::UpdateWarningCertificateInvalid: return "UpdateWarningCertificateInvalid";
        case Error::UpdateWarningSignatureInvalid: return "UpdateWarningSignatureInvalid";
        case Error::UpdateWarningVendorMismatch: return "UpdateWarningVendorMismatch";
        case Error::UpdateWarningClassMismatch: return "UpdateWarningClassMismatch";
        case Error::UpdateWarningDeviceMismatch: return "UpdateWarningDeviceMismatch";
        case Error::UpdateWarningURINotFound: return "UpdateWarningURINotFound";
        case Error::UpdateWarningRollbackProtection: return "UpdateWarningRollbackProtection";
        case Error::UpdateWarningUnknown: return "UpdateWarningUnknown";
        case Error::UpdateErrorWriteToStorage: return "UpdateErrorWriteToStorage";
        case Error::UpdateWarningNoActionRequired: return "UpdateWarningNoActionRequired";
        case Error::UpdateErrorUserActionRequired: return "UpdateErrorUserActionRequired";
        case Error::UpdateFatalRebootRequired: return "UpdateFatalRebootRequired";
        case Error::UpdateErrorInvalidHash: return "UpdateErrorInvalidHash";
        case Error::UpdateWarningBadKeytable: return "UpdateWarningBadKeytable";
        case Error::UpdateCertificateInsertion: return "UpdateCertificateInsertion";
        case Error::UpdateErrorConnection: return "UpdateErrorConnection";

        case Error::EnrollmentErrorBase: return "EnrollmentErrorBase";
        case Error::EnrollmentErrorEnd: return "EnrollmentErrorEnd";

        case Error::CCRBStartFailed: return "CCRBStartFailed";
        case Error::CCRBStopFailed: return "CCRBStopFailed";
        case Error::CCRBInvalidJson: return "CCRBInvalidJson";
        case Error::CCRBCreateM2MObjFailed: return "CCRBCreateM2MObjFailed";
        case Error::CCRBValueTypeError: return "CCRBValueTypeError";
    }
    return "Unrecognized error code";
}

MblError CloudClientError_to_MblError(MbedCloudClient::Error error)
{
    switch (error) {
        case MbedCloudClient::ConnectErrorNone: return Error::None;
        case MbedCloudClient::ConnectAlreadyExists: return Error::ConnectAlreadyExists;
        case MbedCloudClient::ConnectBootstrapFailed: return Error::ConnectBootstrapFailed;
        case MbedCloudClient::ConnectInvalidParameters: return Error::ConnectInvalidParameters;
        case MbedCloudClient::ConnectNotRegistered: return Error::ConnectNotRegistered;
        case MbedCloudClient::ConnectTimeout: return Error::ConnectTimeout;
        case MbedCloudClient::ConnectNetworkError: return Error::ConnectNetworkError;
        case MbedCloudClient::ConnectResponseParseFailed: return Error::ConnectResponseParseFailed;
        case MbedCloudClient::ConnectUnknownError: return Error::ConnectUnknownError;
        case MbedCloudClient::ConnectMemoryConnectFail: return Error::ConnectMemoryConnectFail;
        case MbedCloudClient::ConnectNotAllowed: return Error::ConnectNotAllowed;
        case MbedCloudClient::ConnectSecureConnectionFailed: return Error::ConnectSecureConnectionFailed;
        case MbedCloudClient::ConnectDnsResolvingFailed: return Error::ConnectDnsResolvingFailed;
        case MbedCloudClient::ConnectorFailedToReadCredentials: return Error::ConnectorFailedToReadCredentials;
        case MbedCloudClient::ConnectorInvalidCredentials: return Error::ConnectorInvalidCredentials;
        case MbedCloudClient::ConnectorFailedToStoreCredentials: return Error::ConnectorFailedToStoreCredentials;

        case MbedCloudClient::UpdateWarningCertificateNotFound: return Error::UpdateWarningCertificateNotFound;
        case MbedCloudClient::UpdateWarningIdentityNotFound: return Error::UpdateWarningIdentityNotFound;
        case MbedCloudClient::UpdateWarningCertificateInvalid: return Error::UpdateWarningCertificateInvalid;
        case MbedCloudClient::UpdateWarningSignatureInvalid: return Error::UpdateWarningSignatureInvalid;
        case MbedCloudClient::UpdateWarningVendorMismatch: return Error::UpdateWarningVendorMismatch;
        case MbedCloudClient::UpdateWarningClassMismatch: return Error::UpdateWarningClassMismatch;
        case MbedCloudClient::UpdateWarningDeviceMismatch: return Error::UpdateWarningDeviceMismatch;
        case MbedCloudClient::UpdateWarningURINotFound: return Error::UpdateWarningURINotFound;
        case MbedCloudClient::UpdateWarningRollbackProtection: return Error::UpdateWarningRollbackProtection;
        case MbedCloudClient::UpdateWarningUnknown: return Error::UpdateWarningUnknown;
        case MbedCloudClient::UpdateErrorWriteToStorage: return Error::UpdateErrorWriteToStorage;
        case MbedCloudClient::UpdateWarningNoActionRequired: return Error::UpdateWarningNoActionRequired;
        case MbedCloudClient::UpdateErrorUserActionRequired: return Error::UpdateErrorUserActionRequired;
        case MbedCloudClient::UpdateFatalRebootRequired: return Error::UpdateFatalRebootRequired;
        case MbedCloudClient::UpdateErrorInvalidHash: return Error::UpdateErrorInvalidHash;
        case MbedCloudClient::UpdateWarningBadKeytable: return Error::UpdateWarningBadKeytable;
        case MbedCloudClient::UpdateCertificateInsertion: return Error::UpdateCertificateInsertion;
        case MbedCloudClient::UpdateErrorConnection: return Error::UpdateErrorConnection;
    
        case MbedCloudClient::EnrollmentErrorBase: return Error::EnrollmentErrorBase;
        case MbedCloudClient::EnrollmentErrorEnd: return Error::EnrollmentErrorEnd;
    }
    return Error::Unknown;
}

} // namespace mbl
