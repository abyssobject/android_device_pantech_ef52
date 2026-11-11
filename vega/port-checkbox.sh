#!/bin/bash
# The VEGA Iron 2's check boxes and radio buttons (framework-res pt_btn_check_*
# and pt_btn_radio_*, xxhdpi) for every app: the pictures scaled 2/3 to
# xhdpi (Lanczos in linear light) as zz_ef52_pt_*, the Iron 2's state lists,
# and the framework's own check box / radio drawables (Material and Holo)
# pointed at them. All in the ef52 framework-res overlay (vega/overlay).
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
F=~/android/ref/iron2-fwres/system/framework/framework-res.apk
R=$HERE/overlay/frameworks/base/core/res/res
mkdir -p $R/drawable-xhdpi $R/drawable
W=$(mktemp -d); cd $W
unzip -q -o $F 'res/drawable-xxhdpi/pt_btn_check_*' 'res/drawable-xxhdpi/pt_btn_radio_*'
n=0
for f in res/drawable-xxhdpi/pt_btn_*.png; do
  b=$(basename $f .png)
  convert $f -colorspace RGB -filter Lanczos -resize 66.6667% -colorspace sRGB $R/drawable-xhdpi/zz_ef52_$b.png
  n=$((n+1))
done
echo "$n pictures"
rm -rf $W

# The Iron 2's pt_btn_{check,radio}_holo_{dark,light} state lists
selector() {  # kind theme
  local p=@drawable/zz_ef52_pt_btn_$1
  cat <<EOF
<?xml version="1.0" encoding="utf-8"?>
<!-- ef52: the VEGA Iron 2's pt_btn_$1_holo_$2 (framework-res) -->
<selector xmlns:android="http://schemas.android.com/apk/res/android">
    <item android:state_checked="true" android:state_window_focused="false" android:state_enabled="true" android:drawable="${p}_on_holo_$2" />
    <item android:state_checked="false" android:state_window_focused="false" android:state_enabled="true" android:drawable="${p}_off_holo_$2" />
    <item android:state_checked="true" android:state_pressed="true" android:state_enabled="true" android:drawable="${p}_on_pressed_holo_$2" />
    <item android:state_checked="false" android:state_pressed="true" android:state_enabled="true" android:drawable="${p}_off_pressed_holo_$2" />
    <item android:state_checked="true" android:state_focused="true" android:state_enabled="true" android:drawable="${p}_on_focused_holo_$2" />
    <item android:state_checked="false" android:state_focused="true" android:state_enabled="true" android:drawable="${p}_off_focused_holo_$2" />
    <item android:state_checked="false" android:state_enabled="true" android:drawable="${p}_off_holo_$2" />
    <item android:state_checked="true" android:state_enabled="true" android:drawable="${p}_on_holo_$2" />
    <item android:state_checked="true" android:state_window_focused="false" android:drawable="${p}_on_disabled_holo_$2" />
    <item android:state_checked="false" android:state_window_focused="false" android:drawable="${p}_off_disabled_holo_$2" />
    <item android:state_checked="true" android:state_focused="true" android:drawable="${p}_on_disabled_focused_holo_$2" />
    <item android:state_checked="false" android:state_focused="true" android:drawable="${p}_off_disabled_focused_holo_$2" />
    <item android:state_checked="false" android:drawable="${p}_off_disabled_holo_$2" />
    <item android:state_checked="true" android:drawable="${p}_on_disabled_holo_$2" />
</selector>
EOF
}
for k in check radio; do
  for t in dark light; do
    selector $k $t > $R/drawable/zz_ef52_pt_btn_${k}_holo_$t.xml
    # Holo themes (the KitKat VEGA apps, Holo dialogs)
    selector $k $t > $R/drawable/btn_${k}_holo_$t.xml
  done
  # Material / DeviceDefault (listChoiceIndicator*Material): the VEGA dark set
  selector $k dark > $R/drawable/btn_${k}_material_anim.xml
done
ls $R/drawable | grep -E 'btn_(check|radio)'
