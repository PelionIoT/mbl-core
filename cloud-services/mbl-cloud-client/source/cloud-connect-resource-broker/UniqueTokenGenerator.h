/*
 * Copyright (c) 2019 ARM Ltd.
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


#ifndef UniqueTokenGenerator_h_
#define UniqueTokenGenerator_h_

#include "MblError.h"
#include <string>

namespace mbl {

class UniqueTokenGenerator {
public:

    MblError generate_unique_token(std::string &unique_token);
};

} // namespace mbl

#endif // UniqueTokenGenerator_h_
