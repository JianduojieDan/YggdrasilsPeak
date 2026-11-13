# NVIDIA DLSS in Yggdrasil's Peak (installed 2026-11-13)

Source: NVIDIA's DLSS page (https://developer.nvidia.com/rtx/dlss/get-started) -> "Download UE 5.8 DLSS Plugin":
`2026.10.16_UE5.8_DLSS4.5Plugin_v8.9.0.zip` (1.17 GB, no login needed; NVIDIA RTX SDK EULA in its Documentation folder).
A copy is kept at `Downloads/dlss_ue5.8_v8.9.0/` (not in git).

Installed into `Plugins/` (not in git, 3.3 GB) and enabled in the .uproject:
DLSS (super resolution / DLAA), StreamlineCore, StreamlineDLSSG (frame generation 2x-4x), StreamlineReflex, StreamlineNGXCommon.

To reinstall: unzip, copy those five folders from `Plugins/` of the zip into the project's `Plugins/`, build.

Settings > Graphics (YggSettings::ApplyGraphics): Upscaler DLSS/TSR (r.NGX.DLSS.Enable), Upscaling Auto/Native(=DLAA)/Quality/Balanced/
Performance (r.ScreenPercentage 75 dyn/100/67/58/50), Frame generation Off/2x/3x/4x (r.Streamline.DLSSG.Enable + FramesToGenerate,
DLSS only), Low latency (t.Streamline.Reflex.Enable/Mode, always on with frame generation).
