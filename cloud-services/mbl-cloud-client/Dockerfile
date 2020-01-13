# Copyright (c) 2017-2020 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
# Licensed under the Apache License, Version 2.0 (the License); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an AS IS BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# This Dockerfile is for creating containers in which to run
# mbl-cloud-client outside of mbed Linux (e.g. on a development
# machine) for testing purposes.

from ubuntu:16.04

RUN mkdir -p /config/factory
RUN mkdir -p /config/user
RUN mkdir -p /mnt/flags
RUN mkdir -p /scratch
RUN mkdir -p /pal
RUN mkdir -p /opt/arm

COPY mbl-cloud-client /opt/arm/mbl-cloud-client

CMD ["/bin/bash"]
