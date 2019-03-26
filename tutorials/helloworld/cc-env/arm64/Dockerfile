# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

FROM dockcross/linux-arm64:20190310-7ff84ba

# The following two changes to sources.list are needed because of the upstream
# debian repository changes.
# jessie-backports have been moved to archive.debian.org
# https://lists.debian.org/debian-devel-announce/2019/03/msg00006.html
# https://unix.stackexchange.com/questions/508724/failed-to-fetch-jessie-backports-repository
RUN sed -i -e 's/ftp\.debian\.org/archive\.debian\.org/g' /etc/apt/sources.list
# Delete jessie-updates entry because it doesn't exist anymore
RUN sed -i -e '/jessie-updates/d' /etc/apt/sources.list

# Acquire::Check-Valid-Until=false is needed because archive.debian.org is no longer being updated
RUN apt-get -o Acquire::Check-Valid-Until=false update
# opkg-utils uses "--sort" option of tar. This option was introduced in tar version 1.28.
# We need to use tar from jessie-backports in order to satisfy that.
RUN apt-get install -t jessie-backports tar
RUN wget http://git.yoctoproject.org/cgit/cgit.cgi/opkg-utils/snapshot/opkg-utils-0.3.6.tar.gz && \
    tar xvzf opkg-utils-0.3.6.tar.gz && make -C opkg-utils-0.3.6 install && \
    rm -rf opkg-utils-0.3.6 opkg-utils-0.3.6.tar.gz
ENV DEFAULT_DOCKCROSS_IMAGE linux-arm64
