# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Test swupdate."""

import subprocess


SW_DESCRIPTION = """software =
{
    version = "0.1.0"
        files: (
                {
                    filename = "test";
                    path = "/opt/arm/test";
                }
        );
        scripts: (
                {
                      filename = "shellscript.sh";
                      type = "shellscript";
                }
        );
}
"""


SHELLSCRIPT = """#!/bin/sh
#example type = "shellscript";

do_preinst()
{
    echo "Running preinstall shell script"
    exit 0
}

do_postinst()
{
    echo "Running postinstall shell script"
    exit 0
}

echo $0 $1 > /dev/ttyO0

case "$1" in
preinst)
    echo "call do_preinst"
    do_preinst
    ;;
postinst)
    echo "call do_postinst"
    do_postinst
    ;;
*)
    echo "default"
    exit 1
    ;;
esac
"""


class TestSwupdate:
    """Run swupdate tests."""

    dut_addr = ""

    def test_setup_dut_addr(self, dut_addr):
        """Store the device address."""
        TestSwupdate.dut_addr = dut_addr

        assert dut_addr

    def test_swupdate_calls_shellscript_handler(
        self, execute_helper, tmp_path
    ):
        """Test swupdate installs a dummy payload and calls the handler."""
        payload_path = self._create_swupdate_payload(tmp_path)
        err_code, stdout, stderr = execute_helper.send_mbl_cli_command(
            ["put", "{}".format(str(payload_path)), "/scratch"],
            TestSwupdate.dut_addr,
        )
        assert err_code == 0

        sw_err, sw_out, _ = execute_helper.send_mbl_cli_command(
            ["shell", "/usr/bin/swupdate -vi /scratch/payload.swu"],
            TestSwupdate.dut_addr,
        )

        assert sw_err == 0
        assert "Running preinstall shell script" in sw_out
        assert "Running postinstall shell script" in sw_out

    def _create_swupdate_payload(self, tmp_path):
        sw_desc_path = tmp_path / "sw-description"
        shellscript_path = tmp_path / "shellscript.sh"
        test_file_path = tmp_path / "test"
        payload_path = tmp_path / "payload.swu"

        test_file_path.write_text("TEST")
        sw_desc_path.write_text(SW_DESCRIPTION)
        shellscript_path.write_text(SHELLSCRIPT)

        init_payload_cmd = "echo {} | cpio --format crc -o -F {}".format(
            str(sw_desc_path.name), str(payload_path)
        )
        subprocess.run(
            [init_payload_cmd], check=True, shell=True, cwd=str(tmp_path)
        )
        append_payload_cmd = (
            "echo {file_path} | cpio --append --format crc -o -F "
            "{payload_path}"
        )

        subprocess.run(
            [
                append_payload_cmd.format(
                    file_path=str(test_file_path.name),
                    payload_path=str(payload_path.name),
                )
            ],
            check=True,
            shell=True,
            cwd=str(tmp_path),
        )
        subprocess.run(
            [
                append_payload_cmd.format(
                    file_path=str(shellscript_path.name),
                    payload_path=str(payload_path.name),
                )
            ],
            check=True,
            shell=True,
            cwd=str(tmp_path),
        )

        return payload_path
