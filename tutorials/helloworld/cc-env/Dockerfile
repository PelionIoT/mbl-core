# Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

FROM dockcross/linux-armv7
RUN apt-get update
RUN apt-get install wget
RUN wget http://git.yoctoproject.org/cgit/cgit.cgi/opkg-utils/snapshot/opkg-utils-0.3.6.tar.gz && \
	tar xvzf opkg-utils-0.3.6.tar.gz && make -C opkg-utils-0.3.6 install && \
	rm -rf opkg-utils-0.3.6 opkg-utils-0.3.6.tar.gz
# we use wget because 'apt-get' will install older version of tar which is not compatible with opkg-utils
RUN wget http://ftp.debian.org/debian/pool/main/t/tar/tar_1.30+dfsg-3_amd64.deb && \
	dpkg -i tar_1.30+dfsg-3_amd64.deb && rm -rf tar_1.30+dfsg-3_amd64.deb
ENV DEFAULT_DOCKCROSS_IMAGE linux-armv7
