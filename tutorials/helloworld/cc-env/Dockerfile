# Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

FROM dockcross/linux-armv7
# opkg-utils uses "--sort" option of tar. This option was introduced in tar version 1.28.
# We need to use tar from jessie-backports in order to satisfy that.
RUN echo 'deb http://deb.debian.org/debian jessie-backports main contrib non-free' > /etc/apt/sources.list.d/jessie-backports.list && \
	apt-get update
RUN apt-get install -t jessie-backports tar
RUN apt-get install wget
RUN wget http://git.yoctoproject.org/cgit/cgit.cgi/opkg-utils/snapshot/opkg-utils-0.3.6.tar.gz && \
	tar xvzf opkg-utils-0.3.6.tar.gz && make -C opkg-utils-0.3.6 install && \
	rm -rf opkg-utils-0.3.6 opkg-utils-0.3.6.tar.gz
ENV DEFAULT_DOCKCROSS_IMAGE linux-armv7
