# Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

CC ?= gcc
RM := rm -rf
CP := cp
OPKG_BUILD := opkg-build -Z "xz" -g root -o root

SRC := src/hello_world.c

LIBS :=
COMMON_CFLAGS := -static -Wall -Werror -Wformat=2 -Wunused-parameter -Wwrite-strings -Wconversion
RELEASE_CFLAGS := -O3
DEBUG_CFLAGS := -Og -ftrapv

FILE_EXECUTABLE := hello_world
MACHINE := armv7vet2hf-neon
PACKAGE_VER := 1.0
IPK_PACKAGE := sample-app_$(PACKAGE_VER)_$(MACHINE).ipk

.phony: all debug release test clean

all: debug release
debug: debug/ipk/$(IPK_PACKAGE)
release: release/ipk/$(IPK_PACKAGE)


# Cross-compilation builds
release/$(FILE_EXECUTABLE): $(SRC)
	@echo ' '
	@echo 'Cross-compiling hello_world app for release configuration'
	@mkdir -p $(@D)
	$(CC) $(RELEASE_CFLAGS) $(COMMON_CFLAGS) -o release/$(FILE_EXECUTABLE) $(SRC) -Llib $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

debug/$(FILE_EXECUTABLE): $(SRC)
	@echo ' '
	@echo 'Cross-compiling hello_world app for debug configuration'
	@mkdir -p $(@D)
	$(CC) $(DEBUG_CFLAGS) $(COMMON_CFLAGS) -o debug/$(FILE_EXECUTABLE) $(SRC) -Llib $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '


# Create OCI bundle
release/bundle/rootfs/$(FILE_EXECUTABLE): src_bundle/config.json release/$(FILE_EXECUTABLE)
	@mkdir -p $(@D)
	$(CP) src_bundle/config.json release/bundle/config.json
	$(CP) release/$(FILE_EXECUTABLE) $(@D)/$(FILE_EXECUTABLE)
	@rm release/$(FILE_EXECUTABLE)
	@echo ' '

debug/bundle/rootfs/$(FILE_EXECUTABLE): src_bundle/config.json debug/$(FILE_EXECUTABLE)
	@mkdir -p $(@D)
	$(CP) src_bundle/config.json debug/bundle/config.json
	$(CP) debug/$(FILE_EXECUTABLE) $(@D)/$(FILE_EXECUTABLE)
	@rm debug/$(FILE_EXECUTABLE)
	@echo ' '


# Create IPK
release/ipk/$(IPK_PACKAGE): src_opkg/CONTROL/control release/bundle/rootfs/$(FILE_EXECUTABLE)
	@mkdir -p $(@D)/in/CONTROL
	$(CP) -r release/bundle/* $(@D)/in
	$(CP) -r src_opkg/CONTROL $(@D)/in
	$(OPKG_BUILD) $(abspath $(@D)/in) $(abspath $(@D))
	@echo ' '

debug/ipk/$(IPK_PACKAGE): src_opkg/CONTROL/control debug/bundle/rootfs/$(FILE_EXECUTABLE)
	@mkdir -p $(@D)/in/CONTROL
	$(CP) -r debug/bundle/* $(@D)/in
	$(CP) -r src_opkg/CONTROL $(@D)/in
	$(OPKG_BUILD) $(abspath $(@D)/in) $(abspath $(@D))
	@echo ' '

# Create apps for testing
test:
	@echo 'Create good sample apps'
	for number in 1 2 3 4 5 6; do \
		sed -i "s/Version.*/Version: 1.0/" src_opkg/CONTROL/control; \
		sed -i "s/Package.*/Package: sample-app-$$number-good/" src_opkg/CONTROL/control; \
		$(MAKE) release; \
	done

	@echo 'Create bad architecture app'
	sed -i "s/Package.*/Package: sample-app-3-bad-architecture/" src_opkg/CONTROL/control
	sed -i "s/Version.*/Version: 1.1/" src_opkg/CONTROL/control
	sed -i "s/Architecture.*/Architecture: invalid-architecture/" src_opkg/CONTROL/control
	$(MAKE) release

	@echo 'Create bad OCI runtime app'
	sed -i "s/Package.*/Package: sample-app-4-bad-oci-runtime/" src_opkg/CONTROL/control
	sed -i "s/Architecture.*/Architecture: any/" src_opkg/CONTROL/control
	sed -i "1d" src_bundle/config.json
	$(MAKE) release

clean:
	-$(RM) release/*
	-$(RM) debug/*
	@echo ' '
