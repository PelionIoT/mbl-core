/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "CloudConnectExternalTypes.h"


const char* CloudConnectStatus_to_string(CloudConnectStatus status){

    switch (status)
    {
        case SUCCESS: 
            return "SUCCESS";

        case FAILURE: 
            return "FAILURE";

        default:
            return "Unknown Cloud Connect Status";
    }
}




