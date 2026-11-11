# VEGA Iron 2 (IM-A910S) home launcher "VEGA Home", its icon style agent and
# icon packs, Design Home, the wallpaper popup, the VEGA keyboard, VEGA Music
# with VEGA Sound Effect, File Manager and the clock (alarm, world time, timer,
# stopwatch), ported by los19/launcher/build-launcher19.sh.
#
# Trebuchet stays installed for Recents only (setup19.sh takes its home
# screen out), so VEGA Home is the only home app and the default.
# The VEGA keyboard replaces LatinIME and is the default keyboard; VEGA Music
# replaces Eleven.
LOCAL_PATH := $(call my-dir)

EF44_PANTECH_APPS := \
    PantechLauncher \
    IconStyleAgent \
    HomeDeco \
    WallpaperPopup \
    VegaHomeThemeFacade \
    VegaHomeThemeLimited \
    VegaHomeThemeMiroo \
    VegaHomeThemeUnique \
    VEGAIME \
    VEGAIMEProvider \
    VEGAMusicPlayer \
    VEGAMusicFX \
    FileManager \
    PantechRes \
    VEGAClock \
    WallpaperChooser

# Platform-signed privileged apps, as on the Iron 2. Dexpreopted with the
# system image ("verify"; VEGA Home and the VEGA keyboard "speed", device.mk
# PRODUCT_DEXPREOPT_SPEED_APPS). Without it they ran interpreted until an
# on-device dexopt, which after a ROM update left some of them on stale
# /data/dalvik-cache files ("run-from-apk-fallback"), and the background
# dexopt job never runs while "stay awake" keeps the screen on. The odex
# records the app's class loader context, and ART drops the odex if it differs
# from the one on the phone: the apps whose manifest has
# <uses-library android:name="com.pantech.framework"> (EF44_PANTECH_FW_USERS)
# get it in LOCAL_USES_LIBRARIES (the targetSdk compatibility libraries are
# added by the build). The manifest check itself stays off.
EF44_PANTECH_FW_USERS := HomeDeco PantechLauncher VEGACalculator VEGACalendar VEGAIME VEGAIMEProvider VEGAMms
define ef44-pantech-app
include $$(CLEAR_VARS)
LOCAL_MODULE := $(1)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_SUFFIX := $$(COMMON_ANDROID_PACKAGE_SUFFIX)
LOCAL_SRC_FILES := $(1).apk
LOCAL_CERTIFICATE := platform
LOCAL_PRIVILEGED_MODULE := true
LOCAL_USES_LIBRARIES := $$(if $$(filter $(1),$$(EF44_PANTECH_FW_USERS)),com.pantech.framework)
LOCAL_ENFORCE_USES_LIBRARIES := false
LOCAL_REQUIRED_MODULES := com.pantech.framework com.pantech.framework.xml privapp-permissions-pantech.xml $(3)
LOCAL_OVERRIDES_PACKAGES := $(2)
LOCAL_PREBUILT_JNI_LIBS := $(4)
include $$(BUILD_PREBUILT)
endef

$(eval $(call ef44-pantech-app,VEGAIME,LatinIME,libjni_skyime))
# VEGA Music replaces Eleven (LineageOS music). Its own JNI libraries (album
# art crop, search) come out of its APK.
$(eval $(call ef44-pantech-app,VEGAMusicPlayer,Eleven,VEGAMusicFX,@lib/armeabi/libimagecrop.so @lib/armeabi/liblevenshteindistance.so))
$(eval $(call ef44-pantech-app,VEGAMusicFX,,libvegabundlewrapper))
# File Manager's Pantech widgets take their look from com.pantech.res
# (PantechRes, resources only)
$(eval $(call ef44-pantech-app,FileManager,,PantechRes))
# VEGA Clock is package com.android.deskclock, like LineageOS's clock, which it
# replaces
$(eval $(call ef44-pantech-app,VEGAClock,DeskClock,PantechRes))
# The VEGA wallpapers (WallpaperChooser) replace LineageOS's (Backgrounds)
$(eval $(call ef44-pantech-app,WallpaperChooser,Backgrounds))
# VEGA Calendar (com.android.calendar) replaces LineageOS's (Etar); its
# Pantech text fields and time pickers take their look from PantechRes
$(eval $(call ef44-pantech-app,VEGACalendar,Etar,PantechRes))
# VEGA Calculator (com.pantech.app.calculator) replaces LineageOS's
# (ExactCalculator)
$(eval $(call ef44-pantech-app,VEGACalculator,ExactCalculator,PantechRes))
# VEGA Voice Recorder (the Iron 2's com.pantech.app.voicerecorder, 녹음기)
# replaces LineageOS's (Recorder); its settings' Pantech widgets take their
# look from PantechRes
$(eval $(call ef44-pantech-app,VEGAVoiceRecorder,Recorder,PantechRes))
# VEGA Camera (the Iron 2's com.pantech.app.vegacamera, 카메라) replaces
# LineageOS's (Aperture). Its own JNI libraries (Visidon beauty, ArcSoft best
# shot and smile detection, panorama, live filters) come out of its APK.
EF44_VEGACAMERA_JNI := VDEnhancementAPI-jni adkutils amcm arcpicbest arcplatform \
    arcsoft_detection_base arcsoft_smile_detection jni_panorama nativeJava
$(eval $(call ef44-pantech-app,VEGACamera,Aperture,,$(foreach l,$(EF44_VEGACAMERA_JNI),@lib/armeabi-v7a/lib$(l).so)))
# VEGA Dialer (the Iron 2's com.android.dialer: dial pad, call log,
# favorites; its KitKat in-call screen is taken out) replaces LineageOS's
# (Dialer, the same package). Its Pantech ISP widgets take their pictures from
# com.pantech.isp.res (PantechIspRes, resources only)
$(eval $(call ef44-pantech-app,VEGADialer,Dialer,PantechRes PantechIspRes))
$(eval $(call ef44-pantech-app,PantechIspRes))
# VEGA Contacts (the Iron 2's com.android.contacts, 전화부) replaces LineageOS's
# Contacts (the same package); it shares the Iron 2's contacts code and its
# Pantech ISP widgets with VEGA Dialer
$(eval $(call ef44-pantech-app,VEGAContacts,Contacts,PantechRes PantechIspRes))
# VEGA Messages (the Iron 2's com.pantech.app.mms) replaces LineageOS's
# Messaging. Its messages live in its own store, VEGAMmsSmsProvider (the Iron
# 2's MmsSmsProvider, Pantech's SMS/MMS provider, as vega-sms/vega-mms/
# vega-mms-sms beside Android 12's)
$(eval $(call ef44-pantech-app,VEGAMms,messaging,PantechRes VEGAMmsSmsProvider))
$(eval $(call ef44-pantech-app,VEGAMmsSmsProvider))
EF44_PANTECH_APPS := $(filter-out VEGAIME VEGAMusicPlayer VEGAMusicFX FileManager VEGAClock WallpaperChooser,$(EF44_PANTECH_APPS))
$(foreach app,$(EF44_PANTECH_APPS),$(eval $(call ef44-pantech-app,$(app))))

# VEGA sound effects engine (audio/audio_effects.xml): the Iron 2's
# LifeVibes bundle, effect library API 3.0, loaded by the audio effects HAL.
# It reads its tuning from the fixed path /system/lib/soundfx/.
include $(CLEAR_VARS)
LOCAL_MODULE := libvegabundlewrapper
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
LOCAL_SRC_FILES := libvegabundlewrapper.so
LOCAL_MODULE_RELATIVE_PATH := soundfx
LOCAL_VENDOR_MODULE := true
LOCAL_MULTILIB := 32
LOCAL_STRIP_MODULE := false
LOCAL_CHECK_ELF_FILES := false
LOCAL_REQUIRED_MODULES := libxmlparser LvmDefaultControlParams.xml
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libxmlparser
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
LOCAL_SRC_FILES := libxmlparser.so
LOCAL_VENDOR_MODULE := true
LOCAL_MULTILIB := 32
LOCAL_STRIP_MODULE := false
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := LvmDefaultControlParams.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/soundfx
LOCAL_SRC_FILES := LvmDefaultControlParams.xml
include $(BUILD_PREBUILT)

# VEGA keyboard engine (32-bit, 2014 NDK). Its other dependency, libstlport,
# comes with the ef44 vendor blobs, which are not build modules, so the ELF
# dependency check cannot see it.
include $(CLEAR_VARS)
LOCAL_MODULE := libjni_skyime
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
LOCAL_SRC_FILES := libjni_skyime.so
LOCAL_MULTILIB := 32
LOCAL_STRIP_MODULE := false
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)

# Pantech's framework library (dex jar from the Iron 2 firmware). Dexpreopted
# too: the apps' class loader contexts are built from its dexpreopt.config.
include $(CLEAR_VARS)
LOCAL_MODULE := com.pantech.framework
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := .jar
LOCAL_SRC_FILES := com.pantech.framework.jar
LOCAL_ENFORCE_USES_LIBRARIES := false
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := com.pantech.framework.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/permissions
LOCAL_SRC_FILES := com.pantech.framework.xml
include $(BUILD_PREBUILT)

# The platform privileged permissions the apps request
include $(CLEAR_VARS)
LOCAL_MODULE := privapp-permissions-pantech.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/permissions
LOCAL_SRC_FILES := privapp-permissions-pantech.xml
include $(BUILD_PREBUILT)
