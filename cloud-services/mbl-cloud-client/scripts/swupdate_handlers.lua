-- ----------------------------------------------------------------------------
-- Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
--
-- SPDX-License-Identifier: Apache-2.0
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--     http:#www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.
-- ----------------------------------------------------------------------------


require("swupdate")

-- Utility functions
------------------------------------------------------------------------------
-- Copy the image stream to a file in the /scratch partition.
-- We do this because the current installer scripts can't cope with image
-- streams, they can only handle files at the moment.
-- Returns the image path (or nil on failure) and an exit code.
function copy_image_to_file(image)
    local img_path = string.format("/scratch/%s", tostring(image.filename))
    local err, msg = image:copy2file(img_path)
    if err ~= 0 then
        swupdate.error(
            string.format(
                "Failed to copy image %s to %s: %s",
                tostring(image.filename),
                img_path,
                msg
            )
        )
        return nil, 1
    end
    return img_path, 0
end

-- Remove a file with `rm -rf`
function rm_file(path)
    local cmd = string.format("rm -rf %s", path)
    swupdate.trace("Running command: rm -rf "..path)
    local success, reason, err_code = os.execute(cmd)
    if err_code ~= 0 then
        swupdate.error("Failed to clean up "..path)
        return 1
    end

    return 0
end

-- Run a shell script installer.
-- Runs the given installer with the given image path as its only argument.
-- Logs errors.
function run_installer(installer_path, image_path)
    local cmd = string.format("%s %s", installer_path, image_path)
    swupdate.trace("Running command: "..cmd)
    local success, reason, err = os.execute(cmd)
    if success ~= true then
        local msg = string.format(
            "The command '%s' did not terminate successfully. "..
            "The %s code was '%s'",
            cmd,
            status,
            reason,
            err
        )
        swupdate.error(msg)
        return 1
    end

    if err ~= 0 then
        local msg = string.format(
            "The command '%s' returned an exit code of '%s'. "..
            "The image was not installed correctly.",
            cmd,
            err
        )
        swupdate.error(msg)
        return 1
    end

    return 0
end


------------------------------------------------------------------------------
-- swupdate handlers
------------------------------------------------------------------------------
wks_bootloader1 = function(image)
    local if_err = 0
    local ri_err = 0
    local rm_err = 0
    local exit_code = 0

    local img_path, cp_err = copy_image_to_file(image)
    if cp_err ~= 0 then
        exit_code = 1
        goto cleanup
    end

    ri_err = run_installer("/opt/arm/bootloader_installer.sh", img_path)
    if ri_err ~= 0 then
        exit_code =  1
        goto cleanup
    end

    ::cleanup::
    rm_err = rm_file(img_path)
    if rm_err ~= 0 then
        exit_code = 1
    end

    return exit_code
end


wks_bootloader2 = function(image)
    local if_err = 0
    local ri_err = 0
    local rm_err = 0
    local exit_code = 0

    local img_path, cp_err = copy_image_to_file(image)
    if cp_err ~= 0 then
        exit_code = 1
        goto cleanup
    end

    ri_err = run_installer("/opt/arm/bootloader_installer.sh", img_path)
    if ri_err ~= 0 then
        exit_code = 1
        goto cleanup
    end

    ::cleanup::
    rm_err = rm_file(img_path)
    if rm_err ~= 0 then
        exit_code = 1
    end

    return exit_code
end


boot = function(image)
    local if_err = 0
    local ri_err = 0
    local rm_err = 0
    local exit_code = 0

    local img_path, cp_err = copy_image_to_file(image)
    if cp_err ~= 0 then
        exit_code = 1
        goto cleanup
    end

    ri_err = run_installer("/opt/arm/boot_partition_installer.sh", img_path)
    if ri_err ~= 0 then
        exit_code = 1
        goto cleanup
    end

    ::cleanup::
    rm_err = rm_file(img_path)
    if rm_err ~= 0 then
        exit_code = 1
    end

    return exit_code
end


rootfs = function(image)
    local if_err = 0
    local ri_err = 0
    local rm_err = 0
    local exit_code = 0

    local img_path, cp_err = copy_image_to_file(image)
    if cp_err ~= 0 then
        exit_code = 1
        goto cleanup
    end

    ri_err = run_installer("/opt/arm/rootfs_installer.sh", img_path)
    if ri_err ~= 0 then
        exit_code = 1
        goto cleanup
    end

    ::cleanup::
    rm_err = rm_file(img_path)
    if rm_err ~= 0 then
        exit_code = 1
    end

    return exit_code
end


apps = function(image)
    local if_err = 0
    local ri_err = 0
    local rm_err = 0
    local exit_code = 0

    local img_path, cp_err = copy_image_to_file(image)
    if cp_err ~= 0 then
        exit_code = 1
        goto cleanup
    end

    ri_err = run_installer("/opt/arm/apps_installer.sh", img_path)
    if ri_err ~= 0 then
        exit_code = 1
        goto cleanup
    end

    ::cleanup::
    rm_err = rm_file(img_path)
    if rm_err ~= 0 then
        exit_code = 1
    end

    return exit_code
end


swupdate.register_handler("WKS_BOOTLOADER1v3", wks_bootloader1, swupdate.HANDLER_MASK.IMAGE_HANDLER)
swupdate.register_handler("WKS_BOOTLOADER2v3", wks_bootloader2, swupdate.HANDLER_MASK.IMAGE_HANDLER)
swupdate.register_handler("BOOTv3", boot, swupdate.HANDLER_MASK.IMAGE_HANDLER)
swupdate.register_handler("ROOTFSv3", rootfs, swupdate.HANDLER_MASK.IMAGE_HANDLER)
swupdate.register_handler("APPSv3", apps, swupdate.HANDLER_MASK.IMAGE_HANDLER)
