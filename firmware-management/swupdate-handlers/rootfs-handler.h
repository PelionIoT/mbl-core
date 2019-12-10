/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#ifndef swupdate_handlers_rootfs_handler_h_
#define swupdate_handlers_rootfs_handler_h_

#include "swupdate/swupdate.h"

/**
 * Handler for v4 rootfs images (raw file system images).
 *
 * This handler is NOT automatically registered as a swupdate handler by the
 * swupdate-handlers component. This is done by arm-handlers.c added to the
 * swupdate build by swupdate_%.bbappend in the meta-mbl repo.
 */
int rootfsv4_handler(struct img_type *img, void __attribute__ ((__unused__)) *data);

#endif // swupdate_handlers_rootfs_handler_h_
