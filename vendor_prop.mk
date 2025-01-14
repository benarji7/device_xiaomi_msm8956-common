#
# Copyright (C) 2018 Android Open Source Project
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

# Audio
PRODUCT_PROPERTY_OVERRIDES += \
    af.fast_track_multiplier=1 \
    vendor.audio_hal.period_size=192 \
    audio.deep_buffer.media=true \
    vendor.audio.dolby.ds2.enabled=true \
    vendor.audio.offload.buffer.size.kb=64 \
    vendor.audio.offload.gapless.enabled=true \
    audio.offload.min.duration.secs=30 \
    vendor.audio.offload.multiple.enabled=false \
    vendor.audio.offload.track.enable=false \
    vendor.audio.playback.mch.downsample=true \
    vendor.audio.safx.pbe.enabled=true \
    persist.audio.dirac.speaker=true \
    persist.vendor.audio.fluence.speaker=true \
    persist.vendor.audio.fluence.voicecall=true \
    ro.vendor.qc.sdk.audio.fluencetype=fluence \
    ro.vendor.qc.sdk.audio.ssr=false \
    vendor.audio.tunnel.encode=false \
    vendor.voice.path.for.pcm.voip=true \
    vendor.voice.conc.fallbackpath=deep-buffer \
    vendor.voice.playback.conc.disabled=true \
    vendor.voice.record.conc.disabled=false

# AUX Camera app
PRODUCT_PROPERTY_OVERRIDES += \
    vendor.camera.aux.packagelist=org.codeaurora.snapcam,com.android.camera,org.lineageos.snap

# Bluetooth
PRODUCT_PROPERTY_OVERRIDES += \
    bluetooth.hfp.client=1 \
    persist.vendor.bt.enable.splita2dp=false \
    qcom.bluetooth.soc=smd \
    ro.btconfig.if=smd \
    ro.btconfig.dev=/dev/smd3 \
    ro.btconfig.vendor=qcom \
    ro.btconfig.chip=WCN3680 \
    ro.qualcomm.bt.hci_transport=smd \
    persist.bt.enableAptXHD=true

# Boot
PRODUCT_PROPERTY_OVERRIDES += \
    sys.vendor.shutdown.waittime=500 \
    ro.build.shutdown_timeout=0

# Camera
PRODUCT_PROPERTY_OVERRIDES += \
    media.camera.ts.monotonic=1 \
    persist.camera.HAL3.enabled=1 \
    persist.vendor.qti.telephony.vt_cam_interface=1

# CNE
PRODUCT_PROPERTY_OVERRIDES += \
    persist.cne.feature=1

# Dex2oat use 6 cores during bootup dex compilation
PRODUCT_PROPERTY_OVERRIDES += \
    dalvik.vm.boot-dex2oat-threads=6

# Dex2oat use 4 cores durning normal dex compilation (app installations)
PRODUCT_PROPERTY_OVERRIDES += \
    dalvik.vm.dex2oat-threads=6

# Dex2oat use 4 cores during boot image dex compilation
PRODUCT_PROPERTY_OVERRIDES += \
    dalvik.vm.image-dex2oat-threads=4

# Data modules
PRODUCT_PROPERTY_OVERRIDES += \
    persist.data.iwlan.enable=true \
    persist.data.mode=concurrent \
    persist.data.netmgrd.qos.enable=true \
    ro.use_data_netmgrd=true

# FRP
PRODUCT_PROPERTY_OVERRIDES += \
    ro.frp.pst=/dev/block/bootdevice/by-name/config

# GPS
PRODUCT_PROPERTY_OVERRIDES += \
    persist.gps.qc_nlp_in_use=1 \
    persist.loc.nlp_name=com.qualcomm.location \
    ro.gps.agps_provider=1

# Graphics
PRODUCT_PROPERTY_OVERRIDES += \
    debug.egl.hw=0 \
    debug.enable.sglscale=1 \
    debug.mdpcomp.logs=0 \
    debug.sf.hw=0 \
    dev.pm.dyn_samplingrate=1 \
    persist.demo.hdmirotationlock=false \
    persist.hwc.downscale_threshold=1.15 \
    persist.hwc.mdpcomp.enable=true \
    ro.opengles.version=196610 \
    ro.sf.lcd_density=480

# Media
PRODUCT_PROPERTY_OVERRIDES += \
    media.msm8956hw=1 \
    mm.enable.smoothstreaming=true \
    mmp.enable.3g2=true \
    media.aac_51_output_enabled=true
    

# NITZ
PRODUCT_PROPERTY_OVERRIDES += \
    persist.rild.nitz_plmn=""\
    persist.rild.nitz_long_ons_0=""\
    persist.rild.nitz_long_ons_1="" \
    persist.rild.nitz_long_ons_2=""\
    persist.rild.nitz_long_ons_3=""\
    persist.rild.nitz_short_ons_0=""\
    persist.rild.nitz_short_ons_1=""\
    persist.rild.nitz_short_ons_2=""\
    persist.rild.nitz_short_ons_3=""

# Perf
PRODUCT_PROPERTY_OVERRIDES += \
    ro.vendor.extension_library=libqti-perfd-client.so

# Radio
PRODUCT_PROPERTY_OVERRIDES += \
    DEVICE_PROVISIONED=1 \
    ril.subscription.types=RUIM \
    rild.libargs=-d /dev/smd0 \
    rild.libpath=/vendor/lib64/libril-qc-qmi-1.so \
    ro.telephony.default_network=22,20 \
    telephony.lteOnCdmaDevice=1 \
    persist.dbg.ims_volte_enable=1 \
    persist.dbg.volte_avail_ovr=1 \
    persist.dbg.vt_avail_ovr=1 \
    persist.radio.apm_sim_not_pwdn=1 \
    persist.radio.hw_mbn_update=0 \
    persist.radio.multisim.config=dsds \
    persist.radio.videopause.mode=1 \
    persist.radio.schd.cache=3500 \
    persist.vendor.radio.custom_ecc=1 \
    persist.vendor.radio.rat_on=combine \
    persist.vendor.radio.sib16_support=1 \
    persist.vendor.radio.add_power_save=1 \
    persist.vendor.radio.start_ota_daemon=1 \
    persist.vendor.radio.adb_log_on=1 \
    persist.radio.sw_mbn_update=0 \
    persist.data.qmi.adb_logmask=0 \
    persist.radio.aosp_usr_pref_sel=true

# TCP
PRODUCT_PROPERTY_OVERRIDES += \
    net.tcp.2g_init_rwnd=10

# Time
PRODUCT_PROPERTY_OVERRIDES += \
    persist.timed.enable=true

# WiFi
PRODUCT_PROPERTY_OVERRIDES += \
    wifi.interface=wlan0

# Higher fling velocities for smooth scrolling and to provide better responsiveness - From HalogenOS Team
PRODUCT_PROPERTY_OVERRIDES += \
   ro.min.fling_velocity=140 \
   ro.max.fling_velocity=40000
