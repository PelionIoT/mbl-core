# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Build MBL Client on a Linux PC."""

import os
import sys
import git
import logging
import tempfile
import subprocess
import shutil
import argparse

__version__ = "1.0"

GIT_META_MBL_URL = "git@github.com:ARMmbed/meta-mbl.git"
EXTERNAL_C_FILES = [
    "mbed_cloud_dev_credentials.c",
    "update_default_resources.c",
]

PAL_TARGET = "x86_x64_NativeLinux_mbedtls"
META_MBL_CLIENT_PATCH_DIR = "recipes-connectivity/mbl-cloud-client/files"
PATCHES_LIST = [
    ("arg_too_long_fix_1.patch", "../..", "1"),
    ("arg_too_long_fix_2.patch", "mbed-cloud-client", "1"),
    (
        "linux-paths-update-client-pal-filesystem.patch",
        "mbed-cloud-client",
        "1",
    ),
]


def get_argument_parser():
    """
    Return argument parser.

    :return: parser
    """
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        description="MBL Client builder",
    )

    parser.add_argument(
        "-a",
        "--action",
        choices=["prepare", "configure", "build"],
        default="prepare",
        help="Specify which action to perform",
    )

    parser.add_argument(
        "-m",
        "--meta-mbl-revision",
        default="master",
        help="Specify revision of meta-mbl GIT repository",
    )

    parser.add_argument(
        "-v",
        "--verbose",
        help="Increase output verbosity",
        action="store_true",
    )

    return parser


class ClientBuilder:
    """Builder for MBL Cloud Client."""

    def __init__(self, meta_mbl_revision):
        """Create and initialize ClientBuilder object."""
        self.logger = logging.getLogger("ClientBuilder")
        self.logger.info("Initializing ClientBuilder")
        self.logger.info("Version {}".format(__version__))
        self.meta_mbl_revision = meta_mbl_revision
        self.mbl_cloud_client_directory = os.path.dirname(
            os.path.abspath(__file__)
        )
        self.pal_target_directory = os.path.join(
            self.mbl_cloud_client_directory, "__{}".format(PAL_TARGET)
        )

    def _apply_patches(self, meta_mbl_directory):
        """
        Apply patches to the mbed Cloud Client and PAL code.

        :param meta_mbl_directory: directory where meta-mbl GIT repository were
            download
        """
        for patch_data in PATCHES_LIST:
            patch_name, patch_dir, strip_level = patch_data
            patch_full_path = os.path.join(
                meta_mbl_directory, META_MBL_CLIENT_PATCH_DIR, patch_name
            )
            patch_apply_path = os.path.abspath(
                os.path.join(self.mbl_cloud_client_directory, patch_dir)
            )
            self.logger.info(
                "Applying patch {} cwd: {}".format(
                    patch_full_path, patch_apply_path
                )
            )
            self.logger.debug(
                "Checking if old patch is applied {} on {}".format(
                    patch_full_path, patch_apply_path
                )
            )
            command = [
                "patch",
                "--force",
                "--reverse",
                "--dry-run",
                "--strip={}".format(strip_level),
            ]
            # Check if patch is already applied
            with open(patch_full_path) as patch_file:
                patch_run_status = subprocess.call(
                    command,
                    stdin=patch_file,
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL,
                    cwd=patch_apply_path,
                )
            if patch_run_status == 0:
                # Patch is already applied, we can remove it
                self.logger.debug(
                    "Reversing previously applied patch {} cwd: {}".format(
                        patch_full_path, patch_apply_path
                    )
                )
                command = [
                    "patch",
                    "--force",
                    "--reverse",
                    "--strip={}".format(strip_level),
                ]
                with open(patch_full_path) as patch_file:
                    subprocess.check_call(
                        command, stdin=patch_file, cwd=patch_apply_path
                    )

            self.logger.debug(
                "Applying new patch {} cwd: {}".format(
                    patch_full_path, patch_apply_path
                )
            )
            command = ["patch", "--force", "--strip={}".format(strip_level)]
            with open(patch_full_path) as patch_file:
                subprocess.check_call(
                    command, stdin=patch_file, cwd=patch_apply_path
                )

    def prepare(self):
        """
        Prepare SW tree for compilaiton.

        SW tree preparation includes the following steps:
        1. Check if all external files exists
        2. Download dependancies
        3. Apply patches
        4. Configure PAL
        """
        for external_c_file in EXTERNAL_C_FILES:
            full_path = os.path.join(
                self.mbl_cloud_client_directory, external_c_file
            )
            assert os.access(
                full_path, os.R_OK
            ), "File {} is not accessible".format(full_path)

        # Delete inner mbed-cloud-client tree that downloaded by previous run
        # of this script
        mbed_cloud_client_direcotry = os.path.join(
            self.mbl_cloud_client_directory, "mbed-cloud-client"
        )
        client_dir_exists = os.path.exists(mbed_cloud_client_direcotry)
        client_dir_is_directory = os.path.isdir(mbed_cloud_client_direcotry)
        if client_dir_exists and client_dir_is_directory:
            shutil.rmtree(mbed_cloud_client_direcotry)

        # Download mbed Cloud Client source code from the GIT repository
        command = ["mbed", "deploy", "--protocol", "ssh"]
        self.logger.info("Deploying mbed cloud client")
        subprocess.check_call(command, cwd=self.mbl_cloud_client_directory)

        self.logger.info(
            "Cloning meta-mbl repositoy, switching to branch: {}".format(
                self.meta_mbl_revision
            )
        )
        with tempfile.TemporaryDirectory() as tmp_dir:
            meta_mbl_directory = os.path.join(tmp_dir, "meta-mbl")
            meta_mbl_cloned_repo = git.Repo.clone_from(
                GIT_META_MBL_URL, meta_mbl_directory
            )
            meta_mbl_cloned_repo.git.checkout(self.meta_mbl_revision)
            self._apply_patches(meta_mbl_directory)

        # Configure PAL
        command = [
            "python3",
            "pal-platform/pal-platform.py",
            "deploy",
            "--target={}".format(PAL_TARGET),
            "generate",
        ]
        self.logger.info("Configuring PAL")
        subprocess.check_call(command, cwd=self.mbl_cloud_client_directory)
        self.logger.info("Prepare: done")

    def configure(self):
        """
        Run cmake.

        prepare() method should run before.
        """
        command = [
            "cmake",
            "-G",
            "Unix Makefiles",
            "-DCMAKE_BUILD_TYPE=Debug",
            "-DCMAKE_TOOLCHAIN_FILE=./../pal-platform/Toolchain/GCC/GCC.cmake",
            "-DEXTARNAL_DEFINE_FILE=./../define.txt",
        ]

        self.logger.info("Running cmake")
        subprocess.check_call(command, cwd=self.pal_target_directory)
        self.logger.info("Configure: done")

    def build(self):
        """
        Run make.

        configure() method should run before.
        """
        self.logger.info("Building mbl-cloud-client")
        command = ["make", "mbl-cloud-client"]
        subprocess.check_call(command, cwd=self.pal_target_directory)
        self.logger.info("Build: done")


def _main():
    """Run MBL Client build."""
    parser = get_argument_parser()
    args = parser.parse_args()
    info_level = logging.DEBUG if args.verbose else logging.INFO

    logging.basicConfig(
        level=info_level,
        format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    )
    logger = logging.getLogger("ClientBuilder")
    logger.debug("Command line arguments:{}".format(args))

    client_builder = ClientBuilder(args.meta_mbl_revision)
    if args.action == "prepare":
        client_builder.prepare()
    elif args.action == "configure":
        client_builder.configure()
    elif args.action == "build":
        client_builder.build()
    else:
        assert 0, "Unknown value of 'action' parameter"

    return 0


if __name__ == "__main__":
    sys.exit(_main())
