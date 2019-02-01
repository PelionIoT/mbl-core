#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Provides utilities for reading/writing firmware update HEADER data."""

import hashlib
import struct
import zlib


class FormatError(Exception):
    """Type for firmware update header format errors."""

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

    # The HEADER's firmware_hash field has space for a sha512 hash but the
    # hashes that are currently used are sha256. We'll treat the hashes as
    # sha256s except when they're being read/written from/to binary blobs. When
    # we read the firmware_hash field we'll trim the result down to sha256 size
    # and when we write the firmware_hash field we'll pad the hash up to sha512
    # size.
    _SIZEOF_SHA512 = int(512 / 8)
    _SIZEOF_SHA256 = int(256 / 8)

    _SIZEOF_GUID = int(128 / 8)

    # Fixed number that should always be in a HEADER blob's "magic" field -
    # used to verify that the blob we're given is actually a HEADER blob.
    _HEADER_MAGIC = 0x5A51B3D4

    HEADER_FORMAT_VERSION = 2

    # Note - HEADER format doesn't account for the CRC field or the variable
    # length firmware_signature field. These two fields are at the end of the
    # structure and are dealt with separately.
    _HEADER_FORMAT = ">2I2Q{}s{}sI".format(_SIZEOF_SHA512, _SIZEOF_GUID)
    _CRC_FORMAT = ">I"

    def __init__(self):
        """
        Create a FirmwareUpdateHeader object.

        After creation, the each of the FirmwareUpdateHeader's attributes will
        be zeroed, so in order to create a valid HEADER it is not necessary to
        manually set fields that are not used.
        """
        self._header_struct = struct.Struct(self._HEADER_FORMAT)
        self._header_size = self._header_struct.size

        self._crc_struct = struct.Struct(self._CRC_FORMAT)
        self._crc_size = self._crc_struct.size

        self._header_and_crc_size = self._header_size + self._crc_size

        self.firmware_version = 0
        self.firmware_size = 0
        self.firmware_hash = bytes(self._SIZEOF_SHA256)
        self.campaign_id = bytes(self._SIZEOF_GUID)
        self.firmware_signature = bytes()

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
        if len(data) < self._header_and_crc_size:
            raise FormatError(
                "Data is too short "
                "(expecting at least {}B, received {}B)".format(
                    self._header_and_crc_size, len(data)
                )
            )

        (
            header_magic,
            header_version,
            self.firmware_version,
            self.firmware_size,
            firmware_hash_512,
            self.campaign_id,
            firmware_signature_size,
        ) = self._header_struct.unpack(data[: self._header_size])

        # Cut down the struct's firmware_hash field (which has space for a
        # sha512 hash) to the size of a sha256 hash because we actually only
        # use sha256
        self.firmware_hash = firmware_hash_512[: self._SIZEOF_SHA256]
        if not all(b == 0 for b in firmware_hash_512[self._SIZEOF_SHA256 :]):
            raise FormatError("Invalid sha256 value in firmware_hash field")

        (header_crc,) = self._crc_struct.unpack(
            data[self._header_size : self._header_and_crc_size]
        )

        if header_magic != self._HEADER_MAGIC:
            raise FormatError(
                "Invalid header magic value 0x{:x} (should be 0x{:x})".format(
                    header_magic, self._HEADER_MAGIC
                )
            )

        if header_version != self.HEADER_FORMAT_VERSION:
            raise FormatError(
                "Unsupported header version {}. Version supported: {}".format(
                    header_version, self.HEADER_FORMAT_VERSION
                )
            )

        correct_crc = _calculate_crc(data[: self._header_size])
        if header_crc != correct_crc:
            raise FormatError(
                "Invalid header CRC field {:#x} (should be {:#x})".format(
                    header_crc, correct_crc
                )
            )

        if firmware_signature_size:
            total_size = self._header_and_crc_size + firmware_signature_size
            if len(data) < total_size:
                raise FormatError(
                    "Data is too short "
                    "(expecting at least {}B with {}B for firmware signature; "
                    "received {}B)".format(
                        total_size, firmware_signature_size, len(data)
                    )
                )
            self.firmware_signature = data[
                self._header_and_crc_size : total_size
            ]

    def pack(self):
        """
        Create a binary HEADER blob from this objects attributes.

        Returns:
            bytearray object containing the HEADER blob.

        """
        if len(self.firmware_hash) != self._SIZEOF_SHA256:
            raise FormatError(
                "Firmware hash has incorrect size: "
                "should be {}B, received {}B".format(
                    self._SIZEOF_SHA256, len(self.firmware_hash)
                )
            )

        if len(self.campaign_id) != self._SIZEOF_GUID:
            raise FormatError(
                "Campaign ID has incorrect size: "
                "should be {}B, received {}B".format(
                    self._SIZEOF_GUID, len(self.campaign_id)
                )
            )

        # Pad our sha256 firmware hash to fill up the struct's firmware_hash
        # field which has space for a sha512 hash
        firmware_hash_512 = bytearray(self._SIZEOF_SHA512)
        firmware_hash_512[0 : self._SIZEOF_SHA256] = self.firmware_hash

        data = bytearray(
            self._header_and_crc_size + len(self.firmware_signature)
        )
        self._header_struct.pack_into(
            data,
            0,
            self._HEADER_MAGIC,
            self.HEADER_FORMAT_VERSION,
            self.firmware_version,
            self.firmware_size,
            firmware_hash_512,
            self.campaign_id,
            len(self.firmware_signature),
        )

        self._crc_struct.pack_into(
            data,
            self._header_size,
            _calculate_crc(data[0 : self._header_size]),
        )
        data[self._header_and_crc_size :] = self.firmware_signature
        return data


def calculate_firmware_hash(firmware_stream):
    """Calculate the firmware hash for the given stream."""
    buffer = bytearray(1024 * 128)
    hasher = hashlib.sha256()
    while True:
        read = firmware_stream.readinto(buffer)
        if not read:
            break
        hasher.update(buffer[0:read])
    return hasher.digest()


def _calculate_crc(header_bytes):
    """
    Calculate value of a HEADER blob's CRC field.

    Args:
    * header_bytes(bytes/bytearray): bytes of the HEADER blob up to, but not
      including, the CRC field itself.
    """
    return zlib.crc32(header_bytes) & 0xFFFFFFFF
