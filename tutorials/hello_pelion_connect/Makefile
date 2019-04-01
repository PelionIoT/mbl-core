# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

RM := rm -rf
CP := cp -r
RSYNC := rsync -a
OPKG_BUILD := opkg-build -Z "xz" -g root -o root

RUNTIME_BUNDLE_FILESYSTEM := runtime-bundle-filesystem
MACHINE := any
PACKAGE_VER := 1.0
IPK_PACKAGE := hello-pelion-connect_$(PACKAGE_VER)_$(MACHINE).ipk

.phony: all release clean

all: release
release: release/ipk/$(IPK_PACKAGE)

release/$(RUNTIME_BUNDLE_FILESYSTEM):
	@echo 'Please run the script to create the runtime bundle filesystem:'
	@echo ' sh scripts/dockerize-hello-pelion-connect-app.sh'
	@exit 1

# Create OCI runtime bundle
release/bundle/rootfs/$(RUNTIME_BUNDLE_FILESYSTEM): src_bundle/config.json release/$(RUNTIME_BUNDLE_FILESYSTEM)
	@mkdir -p $(@D)	
	$(CP) src_bundle/config.json release/bundle/config.json
	$(RSYNC) --exclude='dev' release/$(RUNTIME_BUNDLE_FILESYSTEM)/* $(@D)/
	-$(RM) release/$(RUNTIME_BUNDLE_FILESYSTEM)
	@echo ' '
	
# Create IPK
release/ipk/$(IPK_PACKAGE): src_opkg/CONTROL/control release/bundle/rootfs/$(RUNTIME_BUNDLE_FILESYSTEM)
	@mkdir -p $(@D)/in/CONTROL
	@echo $(@D)
	@echo $(abspath $(@D)/in)
	$(CP) release/bundle/* $(@D)/in
	$(CP) src_opkg/CONTROL $(@D)/in
	$(OPKG_BUILD) $(abspath $(@D)/in) $(abspath $(@D))
	@echo ' '


clean:
	-$(RM) release/*
	@echo ' '
