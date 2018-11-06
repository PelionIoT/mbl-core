#
# Copyright (c) 2018 ARM Ltd.
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

FROM dockcross/linux-armv7
RUN apt-get update
RUN apt-get install wget
RUN wget http://git.yoctoproject.org/cgit/cgit.cgi/opkg-utils/snapshot/opkg-utils-0.3.6.tar.gz && \
	tar xvzf opkg-utils-0.3.6.tar.gz && make -C opkg-utils-0.3.6 install && \
	rm -rf opkg-utils-0.3.6 opkg-utils-0.3.6.tar.gz
RUN wget http://ftp.debian.org/debian/pool/main/t/tar/tar_1.30+dfsg-2_amd64.deb && \
	dpkg -i tar_1.30+dfsg-2_amd64.deb && rm -rf tar_1.30+dfsg-2_amd64.deb
ENV DEFAULT_DOCKCROSS_IMAGE linux-armv7
