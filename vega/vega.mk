#
# VEGA resources for the VEGA Iron (ef52) LineageOS 20.0 build
#
# Everything the VEGA S5 (ef44) LineageOS 20.0 work put in its device tree:
# the VEGA Iron 2 apps, sounds, fonts, wallpapers and their settings. The
# framework, SystemUI and Settings parts (VEGA shade, VEGA Settings, dialogs,
# status bar icons, fonts switcher...) are patched into the source tree itself
# by ef44-port/los20/setup20.sh, which setup-ef52.sh runs first.
#
VEGA_PATH := device/pantech/ef52/vega

# VEGA Messages is the default SMS app; the VEGA default wallpaper; the VEGA
# Iron's own specs on Settings > Android version (CPU name)
DEVICE_PACKAGE_OVERLAYS += $(VEGA_PATH)/overlay

# The VEGA Iron's specs on the VEGA "Android version" page (VegaAboutActivity)
PRODUCT_SYSTEM_PROPERTIES += \
    ro.vega.about.resolution=720x1280 \
    ro.vega.about.storage=32GB \
    ro.vega.about.ram=2GB

# VEGA "LED 알림 표시" (lineage-sdk VegaLed, VEGA Settings LED page): the Iron
# has an RGB notification LED
PRODUCT_SYSTEM_PROPERTIES += \
    ro.vega.led=1

# The VEGA Iron 2's status bar signal and no-SIM icons (SystemUI, fixes-ef52.py)
PRODUCT_SYSTEM_PROPERTIES += \
    ro.vega.signal=1

# LineageOS setup wizard with the VEGA Iron 2 setup wizard's look
PRODUCT_SYSTEM_PROPERTIES += \
    ro.vega.setupwizard=1

# Settings panels (internet, volume): the VEGA Iron 2's Wi-Fi icons, on/off
# buttons and seek bars (Settings VegaPanelStyle, fixes-ef52.py)
PRODUCT_SYSTEM_PROPERTIES += \
    ro.vega.panels=1

# VEGA Iron 2 home (VEGA Home), its icon styles and icon packs, Design Home,
# the wallpaper popup, the VEGA keyboard, VEGA Music and Sound Effect, File
# Manager, Clock, Calendar, Calculator, Voice Recorder, Camera, Dialer,
# Contacts and Messages (apps/Android.mk, made by the ef44 los20 launcher
# pipeline). Each overrides its LineageOS counterpart.
PRODUCT_PACKAGES += \
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
    libvegabundlewrapper \
    FileManager \
    PantechRes \
    VEGAClock \
    WallpaperChooser \
    VEGACalendar \
    VEGACalculator \
    VEGAVoiceRecorder \
    VEGACamera \
    VEGADialer \
    PantechIspRes \
    VEGAContacts \
    VEGAMms \
    VEGAMmsSmsProvider

# VEGA Iron 2 sounds in place of LineageOS's ringtones, notification sounds and
# alarms (the source tree's LineageOS lists drop those), with the Iron 2's
# layout under /system/media/audio
PRODUCT_COPY_FILES += \
    $(call find-copy-subdir-files,*,$(VEGA_PATH)/iron2_audio,$(TARGET_COPY_OUT_SYSTEM)/media/audio)
VEGA_IRON2_UI_SOUNDS := Dock Effect_Tick KeypressDelete KeypressReturn KeypressSpacebar \
    KeypressStandard LowBattery Undock VideoRecord camera_click
PRODUCT_COPY_FILES += \
    $(foreach s,$(VEGA_IRON2_UI_SOUNDS),$(VEGA_PATH)/iron2_audio/ui/$(s).ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ui/$(s).ogg) \
    $(VEGA_PATH)/iron2_audio/ui/Camera_Focus.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ui/camera_focus.ogg \
    $(VEGA_PATH)/iron2_lock_sounds/hold_focus_lock.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ui/Lock.ogg \
    $(VEGA_PATH)/iron2_lock_sounds/hold_focus_unlock.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ui/Unlock.ogg
PRODUCT_PRODUCT_PROPERTIES += \
    ro.config.ringtone=VEGA_Ringtone_Jumping.ogg \
    ro.config.notification_sound=Jumping.ogg \
    ro.config.alarm_alert=Heavens_Morning.ogg

# The VEGA lock screen (SystemUI's VegaLockView) counts new missed calls and
# unread messages for its buttons
PRODUCT_COPY_FILES += \
    $(VEGA_PATH)/configs/default-permissions-vega.xml:$(TARGET_COPY_OUT_SYSTEM)/etc/default-permissions/default-permissions-vega.xml

# VEGA Settings names androidx.window.extensions/sidecar as optional
# libraries; with them a phone is not a two-pane large screen
PRODUCT_PACKAGES += \
    androidx.window.extensions \
    androidx.window.sidecar
PRODUCT_SYSTEM_PROPERTIES += \
    persist.sys.fflag.override.settings_support_large_screen=false

# VEGA Home and the VEGA keyboard are always running: compiled ahead of time
PRODUCT_DEXPREOPT_SPEED_APPS += \
    PantechLauncher \
    VEGAIME

# VEGA wallpapers WallpaperPopup restores and saves from
PRODUCT_COPY_FILES += \
    $(foreach f,$(wildcard $(VEGA_PATH)/apps/home_default/*.jpg),\
        $(f):$(TARGET_COPY_OUT_SYSTEM)/media/images/hold/home_default/$(notdir $(f)))

# VEGA Iron 2 fonts for Settings > Display > Font
PRODUCT_COPY_FILES += \
    $(foreach f,$(wildcard $(VEGA_PATH)/fonts/*.ttf),\
        $(f):$(TARGET_COPY_OUT_SYSTEM)/fonts/$(notdir $(f)))
