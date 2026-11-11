#
# Copyright (C) 2016 The LineageOS Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import common

def FullOTA_InstallBegin(info):
    # ef52: no backuptool.sh (addon.d) backup/restore; GApps are flashed
    # separately after the ROM when wanted.
    common.OPTIONS.backuptool = False

def FullOTA_InstallEnd(info):
    # Mount the system-as-root image away from /system: TWRP's own shell
    # lives under /system/bin and would be shadowed by the ROM.
    dev = "/dev/block/platform/msm_sdcc.1/by-name/system"
    info.script.AppendExtra('run_program("/sbin/umount", "/system");')
    info.script.AppendExtra(
        'mount("ext4", "EMMC", "%s", "/mnt/system", "");' % dev)
    info.script.AppendExtra(
        'ifelse(run_program("/sbin/sh", "/mnt/system/system/bin/device_check.sh", '
        '"/mnt/system/system") == 0, ui_print("EF52 firmware variant installed"), '
        'ifelse(run_program("/system/bin/sh", "/mnt/system/system/bin/device_check.sh", '
        '"/mnt/system/system") == 0, ui_print("EF52 firmware variant installed"), '
        'abort("E3001: device_check.sh failed (EF52 firmware variant)")));')
    info.script.AppendExtra('unmount("/mnt/system");')
