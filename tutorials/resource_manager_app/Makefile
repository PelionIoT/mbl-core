# Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

RM := rm -rf
CP := cp
OPKG_BUILD := opkg-build -Z "xz" -g root -o root 


MACHINE := armv7vet2hf-neon
PACKAGE_VER := 1.0
IPK_PACKAGE := resource-manager-sample-app-package_$(PACKAGE_VER)_$(MACHINE).ipk

.phony: all clean

all: ipk/$(IPK_PACKAGE)

# Create OCI bundle
bundle/rootfs/*: src_bundle/config.json
	@mkdir -p $(@D)
	$(CP) src_bundle/config.json bundle/config.json
	@echo ' '


# Create IPK
ipk/$(IPK_PACKAGE): src_opkg/CONTROL/control bundle/rootfs/*
	@mkdir -p $(@D)/in/CONTROL
	$(CP) -r bundle/* $(@D)/in
	$(CP) -r src_opkg/CONTROL $(@D)/in
	$(OPKG_BUILD) $(abspath $(@D)/in) $(abspath $(@D))
	@echo ' '


clean:
	-$(RM) bundle
	-$(RM) ipk
	@echo ' '

