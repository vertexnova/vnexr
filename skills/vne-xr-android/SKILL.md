---
name: vne-xr-android
description: >
  Android OpenXR APK packaging, Galaxy XR install, logcat, and scrcpy for vnexr.
  Use when editing Gradle templates, build_android.sh, or device demo steps.
---

# XR Android Packaging

## Build / install

```bash
./scripts/build_android.sh -t Debug -a install \
  -sample sample_02_openxr_stereo -abi arm64-v8a
```

Package id pattern: `com.vertexnova.vnexr.<flavor>` (e.g. `openxr_stereo`).

## Manifest checklist

- `android.software.xr.api.openxr` feature required
- Immersive HMD intent category: `org.khronos.openxr.intent.category.IMMERSIVE_HMD`
- Full-space start mode meta-data for unmanaged OpenXR activities
- OpenXR runtime broker `<queries>`
- NativeActivity + `android.app.lib_name` pointing at the sample SHARED lib
- Package the OpenXR loader `.so` built with the app (FetchContent / NDK link)

Templates live under `samples/framework/app/platform/android/gradle/`.
Generator: `cmake/VneXrAndroidGradle.cmake`.

## Device debug

```bash
adb devices
./scripts/verify_android_vulkan.sh com.vertexnova.vnexr.openxr_stereo
adb logcat -s vne.xr:* OpenXR:* vulkan:*
```

## Audience mirror

```bash
scrcpy --video-codec=h264 --max-size 1280
```

Record a clean run as fallback if the meeting room cannot mirror the headset.
