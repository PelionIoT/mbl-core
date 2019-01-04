#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

import hashlib
import struct
import zlib


def _calculate_crc(header_bytes):
    """
    Calculate value of a HEADER blob's CRC field.

    Args:
    * header_bytes(bytes/bytearray): bytes of the HEADER blob up to, but not
      including, the CRC field itself.
    """

    return zlib.crc32(header_bytes) & 0xffffffff


class FormatError(Exception):
    """Type for firmware update header format errors"""

    pass


class FirmwareUpdateHeader:
    """
    Class to represent the information in firmware update header blobs.

    When the cloud client gives MBL's update scripts a firmware update payload
    to install, it also gives the scripts a "HEADER" file that contains some
    information about the update.

    The (binary) format of the HEADER file can be found at
    https://github.com/ARMmbed/update-client-hub/blob/master/modules/common/update-client-common/arm_uc_metadata_header_v2.h#L54.

    This class can be used to encode and decode the data in these HEADER files.

    Attributes:
    * firmware_version (int): version of the firmware in a firmware payload
      (really a UNIX timestamp).
    * firmware_size (int): size of the firmware payload in bytes.
    * firmware_hash (bytes): SHA512 hash of the firmware payload.
    * campaign_id (bytes): The GUID of the update campaign.
    * firmware_signature (bytes): opaque blob for a firmware's signature.

    Class constants:
    * HEADER_FORMAT_VERSION (int): version of HEADER format that this class
      supports.
    """

    __SIZEOF_SHA512 = int(512 / 8)
    __SIZEOF_GUID = int(128 / 8)
    __HEADER_MAGIC = 0x5a51b3d4

    HEADER_FORMAT_VERSION = 2

    # Note - HEADER format doesn't account for the CRC field or the variable
    # length firmware_signature field. These two fields are at the end of the
    # structure and are dealt with separately.
    __HEADER_FORMAT = ">2I2Q{}s{}sI".format(__SIZEOF_SHA512, __SIZEOF_GUID)
    __CRC_FORMAT = ">I"

    def __init__(self):
        """
        Create a FirmwareUpdateHeader object.

        After creation, the each of the FirmwareUpdateHeader's attributes will be
        zeroed, so in order to create a valid HEADER it is not necessary to
        manually set fields that are not used.
        """

        self.__header_struct = struct.Struct(self.__HEADER_FORMAT)
        self.__header_size = self.__header_struct.size

        self.__crc_struct = struct.Struct(self.__CRC_FORMAT)
        self.__crc_size = self.__crc_struct.size

        self.__header_and_crc_size = self.__header_size + self.__crc_size

        self.firmware_version = 0
        self.firmware_size = 0
        self.firmware_hash = bytes(self.__SIZEOF_SHA512)
        self.campaign_id = bytes(self.__SIZEOF_GUID)
        self.firmware_signature = bytes(0)

    def unpack(self, data):
        """
        Set this object's attributes from a given HEADER blob.

        Notes:
        * The version of the format of the HEADER blob must be 2

        * The data passed may be longer than required - any data after the
          HEADER is ignored.

        Args:
        * data (bytes/bytearray): the binary data of a HEADER file.

        Returns: None
        """

        if len(data) < self.__header_and_crc_size:
            raise FormatError(
                "Data is too short (expecting at least {}B)".format(
                    self.__header_and_crc_size
                )
            )

        (
            header_magic,
            header_version,
            self.firmware_version,
            self.firmware_size,
            self.firmware_hash,
            self.campaign_id,
            firmware_signature_size,
        ) = self.__header_struct.unpack(data[: self.__header_size])

        (header_crc,) = self.__crc_struct.unpack(
            data[self.__header_size : self.__header_and_crc_size]
        )

        if header_magic != self.__HEADER_MAGIC:
            raise FormatError(
                "Invalid header magic value 0x{:x} (should be 0x{:x})".format(
                    header_magic, self.__HEADER_MAGIC
                )
            )

        if header_version != self.HEADER_FORMAT_VERSION:
            raise FormatError(
                "Unsupported header version {}".format(header_version)
            )

        correct_crc = _calculate_crc(data[: self.__header_size])
        if header_crc != correct_crc:
            raise FormatError(
                "Invalid header CRC field {:#x} (should be {:#x})".format(
                    header_crc, correct_crc
                )
            )

        if firmware_signature_size:
            total_size = self.__header_and_crc_size + firmware_signature_size
            if len(data) < total_size:
                raise FormatError(
                    "Data is too short (expecting at least {}B with {}B for firmware signature)".format(
                        total_size, firmware_signature_size
                    )
                )
            self.firmware_signature = data[
                self.__header_and_crc_size : total_size
            ]

    def pack(self):
        """
        Create a binary HEADER blob from this objects attributes.

        Returns:
            bytearray object containing the HEADER blob.
        """
        data = bytearray(
            self.__header_and_crc_size + len(self.firmware_signature)
        )
        self.__header_struct.pack_into(
            data,
            0,
            self.__HEADER_MAGIC,
            self.HEADER_FORMAT_VERSION,
            self.firmware_version,
            self.firmware_size,
            self.firmware_hash,
            self.campaign_id,
            len(self.firmware_signature),
        )

        self.__crc_struct.pack_into(
            data,
            self.__header_size,
            _calculate_crc(data[0 : self.__header_size]),
        )
        data[self.__header_and_crc_size :] = self.firmware_signature
        return data


def calculate_firmware_hash(firmware_stream):
    """
    Calculate the firmware hash for the given stream.
    """
    buffer = bytearray(1024 * 128)
    hasher = hashlib.sha256()
    while True:
        read = firmware_stream.readinto(buffer)
        if not read:
            break
        hasher.update(buffer[0:read])
    return hasher.digest()
