/*
 * Copyright (c) 2017 ARM Ltd.
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

#ifndef mbl_log_h_
#define mbl_log_h_

#include "MblError.h"


/**
 * A signal handler to tell us to reopen the log file. Intended to be
 * invoked when the log file is rotated (by e.g. logrotate).
 */
extern "C" void mbl_log_reopen_signal_handler(int signal);

namespace mbl {

/**
 * Initialize the log and trace mechanisms. After calling this the mbed-trace
 * library can be used for logging.
 */
MblError log_init();

} // namespace mbl

#endif // mbl_log_h_
