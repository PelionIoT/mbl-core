#!/usr/bin/env python
# SPDX-License-Identifier: Apache-2.0

__version__ = '1.0'

"""
This script generates test files for testing ARM MBL App Manager using pytest.
Test files consist of pairs of .ipk and .json config files.
Output directory "test_files" holds several of such pairs.
The output directory "test_files" should be pushed to device to /home/app/
This script generate 4 test cases: 2 positive tests and 2 negative tests.
"""

import sys
import argparse
import os
import subprocess
import logging
import hashlib
import json

DATA_DIR_NAME = "DATA"
CONTROL_DIR_NAME = "CONTROL"
IPKG_DIR_NAME = "IPKG"
OUTPUT_FILES_DIR_NAME = "test_files"


class AppManagerTestCaseConfig(object):
    """
    AppManagerTestCaseConfig contains configuration data of how to create
    test case files.
    It is used as an input parameter of create_test_files() function of
    AppManagerTestCaseGenerator class below.
    """
    def __init__(
            self,
            output_dir,
            package_name,
            package_description,
            package_version,
            package_architecture,
            package_files,
            return_code_install,
            return_code_remove,
            ctrl_add_package_name=True
    ):
        self.output_dir = output_dir
        self.package_name = package_name
        self.package_description = package_description
        self.package_version = package_version
        self.package_architecture = package_architecture
        self.package_files = package_files
        self.return_code_install = return_code_install
        self.return_code_remove = return_code_remove
        self.ctrl_add_package_name = ctrl_add_package_name

        # Init directories names
        self.package_dest_dir = \
            os.path.join(self.output_dir, package_name, "")
        self.data_dir = \
            os.path.join(self.package_dest_dir, DATA_DIR_NAME)
        self.control_dir = \
            os.path.join(self.package_dest_dir, CONTROL_DIR_NAME)
        self.ipkg_dir = \
            os.path.join(self.package_dest_dir, IPKG_DIR_NAME, "")
        self.test_files_dir = \
            os.path.join(output_dir, OUTPUT_FILES_DIR_NAME, "")
        # output_filename_no_postfix is used for .ipk and .json output files
        self.output_filename_no_postfix = "{}_{}_{}".format(
            package_name,
            package_version,
            package_architecture
        )


class AppManagerTestCaseGenerator(object):
    def __init__(self, _logger):
        self.logger = _logger
        self.config = None

    def _generate_ipk_content(self):
        # Create DATA directory that contains several files
        for filename in self.config.package_files:
            filename_full_path = os.path.join(
                self.config.data_dir,
                filename)
            os.makedirs(os.path.dirname(filename_full_path), exist_ok=True)
            with open(filename_full_path, "w") as f:
                f.write("File: {}, Description: {}".format(
                    filename,
                    self.config.package_description
                ))

        # Create CONTROL
        filename = os.path.join(self.config.control_dir, "control")
        os.makedirs(os.path.dirname(filename), exist_ok=True)
        with open(filename, "w") as f:
            if self.config.ctrl_add_package_name:
                f.write("Package: {}\n".format(self.config.package_name))
            f.write("Source: ARM\n")
            f.write("Version: {}\n".format(self.config.package_version))
            f.write("Section: base\n")
            f.write("Priority: optional\n")
            f.write("Maintainer: ARM\n")
            f.write("Architecture: {}\n".format(
                self.config.package_architecture
            ))
            f.write("Description: {}\n\n".format(
                self.config.package_description
            ))
        self.logger.info(
            "Successfully generated ipk content for package: {}".format(
                         self.config.package_name)
        )

    def _create_ipk_file(self):
        # Create ipk content
        self._generate_ipk_content()

        # Create IPKG directory
        os.makedirs(os.path.dirname(self.config.ipkg_dir), exist_ok=True)

        # Create tar files for DATA and CONTROL
        command = [
            "tar",
            "-cvzf",
            os.path.join(self.config.ipkg_dir, "data.tar.gz"),
            "."
        ]
        subprocess.check_call(command, cwd=self.config.data_dir)

        command = [
            "tar",
            "-cvzf",
            os.path.join(self.config.ipkg_dir, "control.tar.gz"),
            "."
        ]
        subprocess.check_call(command, cwd=self.config.control_dir)

        # Create ipk file
        os.makedirs(os.path.dirname(self.config.test_files_dir), exist_ok=True)
        ipk_file_name = os.path.join(
                self.config.test_files_dir,
                "{}.ipk".format(self.config.output_filename_no_postfix)
        )
        command = [
            "tar",
            "-cvzf",
            ipk_file_name,
            "data.tar.gz",
            "control.tar.gz"
        ]

        subprocess.check_call(command, cwd=self.config.ipkg_dir)

    def _create_test_case_config(self):
        """
        Generates ipk JSON file describing the test case, expected results
        and md5 value for each file under package DATA dir.
        JSON file is saved to self.info.test_files_dir

        JSON test case config structure:
        {
            "files": {
                "<filename_1>": "<md5 hash value>"
                "<filename_2>": "<md5 hash value>"
            },
            "ipk_filename": "<package_ipk_filename>",
            "package_architecture": "<architecture>",
            "package_description": "<description>",
            "package_version": "<version>",
            "return_code_install": <int>,
            "return_code_remove": <int>
        }

        :return: None
        """

        # List all files in DATA dir along with their md5 hash
        files_dict = dict()
        for subdir, dirs, files in os.walk(self.config.data_dir):
            for file in files:
                file_full_path = os.path.join(subdir, file)
                with open(file_full_path, "rb") as infile:
                    # Calculate md5 hash
                    buff = infile.read()
                    md5_hash = hashlib.md5(buff).hexdigest()
                    subdir_relative_path = \
                        os.path.relpath(subdir, self.config.data_dir)
                    filename_relative_path = \
                        os.path.join(subdir_relative_path, file).lstrip("./")
                    files_dict[filename_relative_path] = md5_hash
        # Build JSON dictionary
        json_dict = dict()
        json_dict["ipk_filename"] = "{}.ipk".format(
            self.config.output_filename_no_postfix)
        json_dict["package_name"] = self.config.package_name
        json_dict["package_description"] = self.config.package_description
        json_dict["package_architecture"] = self.config.package_architecture
        json_dict["package_version"] = self.config.package_version
        json_dict["return_code_install"] = self.config.return_code_install
        json_dict["return_code_remove"] = self.config.return_code_remove
        json_dict["files"] = files_dict

        json_file_name = os.path.join(
                self.config.test_files_dir,
                "{}.json".format(self.config.output_filename_no_postfix)
        )

        with open(json_file_name, "w") as outfile:
            json.dump(json_dict, outfile, indent=4, sort_keys=True)

        self.logger.info("Successfully generated JSON file: {}".format(
                         json_file_name))

    def create_test_files(self, test_case_config):
        """
        Creates ipk files and corresponding config file based on
        test_case_config
        :param test_case_config: Test case config
        :return: None
        """
        self.config = test_case_config
        # Create test files directory holds all ipks for testing
        os.makedirs(os.path.dirname(self.config.test_files_dir), exist_ok=True)
        try:
            self._create_ipk_file()
            self._create_test_case_config()
        except subprocess.CalledProcessError as e:
            self.logger.exception(
                "Operation failed with return error code: {}".format(
                    e.returncode)
            )
        except OSError:
            self.logger.exception(
                "Operation failed with OSError")


def get_argument_parser():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        description='ARM App manager test case generator'
    )
    parser.add_argument(
        '-o',
        '--output-directory',
        metavar='OUTPUT',
        required=True,
        help='Output directory of test files'
    )

    parser.add_argument(
        "-v", "--verbose",
        help="increase output verbosity",
        action="store_true")
    return parser


def _main():
    parser = get_argument_parser()
    args = parser.parse_args()

    debug_level = logging.INFO

    if args.verbose is not None:
        if args.verbose:
            debug_level = logging.DEBUG
        else:
            debug_level = logging.INFO

    logging.basicConfig(
        level=debug_level,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    )
    logger = logging.getLogger('ARM APP MANAGER TEST CASE GENERATOR')
    logger.debug('Starting ARM APP MANAGER TEST CASE GENERATOR: ' +
                 __version__)
    logger.info('Command line arguments:{}'.format(args))

    app_manager_test_case_generator = AppManagerTestCaseGenerator(logger)

    # Create 2 valid ipks
    package_files = ["Dummy_file1.txt", "foo/Dummy_file2.txt"]
    test_case_config1 = AppManagerTestCaseConfig(
        output_dir=args.output_directory,
        package_name="valid-ipk1",
        package_description="This is a valid ipk1 description.\n"
                            "Expecting to successfully installed.",
        package_version="1.0",
        package_architecture="armv7vet2hf-neon",
        package_files=package_files,
        return_code_install=0,
        return_code_remove=0
    )
    app_manager_test_case_generator.create_test_files(test_case_config1)

    package_files = [
        "Dummy_file1.txt",
        "foo/Dummy_file2.txt",
        "foo/123/Dummy_file3.txt"
    ]
    test_case_config2 = AppManagerTestCaseConfig(
        output_dir=args.output_directory,
        package_name="valid-ipk2",
        package_description="This is a valid ipk2 description.\n"
                            "Expecting to successfully installed.",
        package_version="2.0",
        package_architecture="armv7vet2hf-neon",
        package_files=package_files,
        return_code_install=0,
        return_code_remove=0
    )
    app_manager_test_case_generator.create_test_files(test_case_config2)

    # Create 2 invalid ipks
    package_files = ["Dummy_file1.txt", "foo/Dummy_file2.txt"]
    test_case_config3 = AppManagerTestCaseConfig(
        output_dir=args.output_directory,
        package_name="invalid-ipk1",
        package_description="Invalid ipk1 file (missing package name).",
        package_version="2.0",
        package_architecture="armv7vet2hf-neon",
        package_files=package_files,
        return_code_install=1,
        return_code_remove=0,
        ctrl_add_package_name=False
    )
    app_manager_test_case_generator.create_test_files(test_case_config3)

    package_files = ["Dummy_file1.txt"]
    test_case_config4 = AppManagerTestCaseConfig(
        output_dir=args.output_directory,
        package_name="invalid-ipk2",
        package_description="Invalid ipk2 file (unsupported architecture).",
        package_version="2.0",
        package_architecture="invalid-architecture",
        package_files=package_files,
        return_code_install=1,
        return_code_remove=0
    )
    app_manager_test_case_generator.create_test_files(test_case_config4)


if __name__ == '__main__':
    sys.exit(_main())
