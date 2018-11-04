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
IPK_PACKAGE := user-sample-app-package_$(PACKAGE_VER)_$(MACHINE).ipk

.phony: all debug release clean

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

clean:
	-$(RM) release/*
	-$(RM) debug/*
	@echo ' '
