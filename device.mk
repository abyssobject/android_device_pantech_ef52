#
# Copyright (C) 2011 The Android Open-Source Project
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

$(call inherit-product, $(SRC_TARGET_DIR)/product/languages_full.mk)

$(call inherit-product, vendor/pantech/ef52/ef52-vendor.mk)

PRODUCT_SOONG_NAMESPACES += device/pantech/ef52/nfc/legacy

DEVICE_PACKAGE_OVERLAYS += device/pantech/ef52/overlay

# Device uses high-density artwork where available
PRODUCT_AAPT_CONFIG := normal hdpi xhdpi
PRODUCT_AAPT_PREF_CONFIG := xhdpi

# Camera
#PRODUCT_PACKAGES += \
#    camera.msm8960

# Lights
PRODUCT_PACKAGES += \
	lights.msm8960

# LiveDisplay
PRODUCT_PACKAGES += \
    vendor.lineage.livedisplay@2.0-service.ef52

# audio UCM files
#PRODUCT_COPY_FILES += \
                       device/pantech/ef52/snd_soc_msm/snd_soc_msm_2x_Fusion3:system/etc/snd_soc_msm/snd_soc_msm_2x_Fusion3 \

# Checking model
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/releasetools/device_check.sh:system/bin/device_check.sh

# keylayout
PRODUCT_COPY_FILES += \
	device/pantech/ef52/keylayout/AVRCP.kl:system/usr/keylayout/AVRCP.kl \
	device/pantech/ef52/keylayout/Generic.kl:system/usr/keylayout/Generic.kl \
	device/pantech/ef52/keylayout/gpio-keys.kl:system/usr/keylayout/gpio-keys.kl \
	device/pantech/ef52/keylayout/qwerty.kl:system/usr/keylayout/qwerty.kl \
	device/pantech/ef52/keylayout/apq8064-tabla-snd-card_Button_Jack.kl:system/usr/keylayout/apq8064-tabla-snd-card_Button_Jack.kl 

# keychars
PRODUCT_COPY_FILES += \
	device/pantech/ef52/keychars/Generic.kcm:system/usr/keychars/Generic.kcm \
	device/pantech/ef52/keychars/qwerty2.kcm:system/usr/keychars/qwerty2.kcm \
	device/pantech/ef52/keychars/qwerty.kcm:system/usr/keychars/qwerty.kcm \
	device/pantech/ef52/keychars/Virtual.kcm:system/usr/keychars/Virtual.kcm

# idc
PRODUCT_COPY_FILES += \
	device/pantech/ef52/idc/qwerty2.idc:system/usr/idc/qwerty2.idc \
	device/pantech/ef52/idc/qwerty.idc:system/usr/idc/qwerty.idc \

PRODUCT_COPY_FILES += \
	device/pantech/ef52/thermald/thermald-8064_ef52.conf:$(TARGET_COPY_OUT_VENDOR)/etc/thermald.conf \
	device/pantech/ef52/thermald/thermald-8064_ef52.conf:$(TARGET_COPY_OUT_VENDOR)/etc/thermald-8064.conf \
	device/pantech/ef52/thermald/thermald-8064ab_ef52.conf:$(TARGET_COPY_OUT_VENDOR)/etc/thermald-8064ab.conf

PRODUCT_PROPERTY_OVERRIDES += \
	ro.sf.lcd_density=320

# Inherit from msm8960-common
$(call inherit-product, device/pantech/msm8960-common/msm8960.mk)

# PN544 uses the legacy NXP HCI stack and the proprietary board HAL.
PRODUCT_PACKAGES += NfcPn544 Tag
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.nfc.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.nfc.xml

# Reject early modem requests until the stock QCRIL has initialized QMI.
PRODUCT_PACKAGES += libril_ef52

# VEGA resources (apps, sounds, fonts, wallpapers, VEGA Iron specs)
$(call inherit-product, device/pantech/ef52/vega/vega.mk)

# Bluetooth profiles. Android 13's AdapterService starts a profile only when
# its bluetooth.profile.*.enabled property is true; without any, only GATT
# ran: devices paired but A2DP/HFP/HID never connected ("연결" did nothing).
# Classic profiles of the WCN3660 (BT 4.0, no LE Audio).
PRODUCT_VENDOR_PROPERTIES += \
    bluetooth.profile.a2dp.source.enabled=true \
    bluetooth.profile.avrcp.target.enabled=true \
    bluetooth.profile.bas.client.enabled=true \
    bluetooth.profile.gatt.enabled=true \
    bluetooth.profile.hfp.ag.enabled=true \
    bluetooth.profile.hid.device.enabled=true \
    bluetooth.profile.hid.host.enabled=true \
    bluetooth.profile.map.server.enabled=true \
    bluetooth.profile.opp.enabled=true \
    bluetooth.profile.pan.nap.enabled=true \
    bluetooth.profile.pan.panu.enabled=true \
    bluetooth.profile.pbap.server.enabled=true

# Setup wizard: LineageOS's own, also with GApps flashed afterwards (user's
# choice). LineageSetupWizard finishes at once when Google's SetupWizard is
# installed (hasGMS), so that one is kept off the users (sysconfig).
PRODUCT_COPY_FILES += \
    device/pantech/ef52/configs/sysconfig/ef52-lineage-setupwizard.xml:$(TARGET_COPY_OUT_SYSTEM)/etc/sysconfig/ef52-lineage-setupwizard.xml

# VEGA camera dialog pieces (ef52-port/camera, RRO; the APK is unchanged)
PRODUCT_PACKAGES += Ef52VegaCameraOverlay

# Verbose boot: boot log lines over the boot animation (fixes-ef52.py)
PRODUCT_PRODUCT_PROPERTIES += ro.vega.verbose_boot=true

# Settings home: 15dp gap under a folded category, as on the Iron 2
PRODUCT_PRODUCT_PROPERTIES += ro.vega.home_fold_gap=true

# Bluetooth music: A2DP software queue 20 RTP frames (~87ms, AOSP 10 = ~43ms)
# against UNDERFLOW when audioserver stalls (fixes-ef52.py, BT audio HAL)
PRODUCT_VENDOR_PROPERTIES += ro.vega.bt_a2dp_mq_frames=20

# ef52 (Adreno 320) SurfaceFlinger GPU composition: framebuffer EGLImage
# cache >= FramebufferSurface buffers, own texture + FBO per output buffer
PRODUCT_SYSTEM_PROPERTIES += \
    ro.vega.sf_fb_image_cache=4 \
    ro.vega.sf_fbo_per_image=1
