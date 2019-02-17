# Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

FROM debian:9.7
 
RUN apt-get update && apt-get install wget build-essential -y
RUN wget http://git.yoctoproject.org/cgit/cgit.cgi/opkg-utils/snapshot/opkg-utils-0.3.6.tar.gz && \
	tar xvzf opkg-utils-0.3.6.tar.gz && make -C opkg-utils-0.3.6 install && \
	rm -rf opkg-utils-0.3.6 opkg-utils-0.3.6.tar.gz
RUN apt-get install sudo

CMD ["/resource_manager_app/create_package.sh"]
