---
name: vne-xr-android
description: >
  Android OpenXR APK packaging, Galaxy XR install, logcat, and scrcpy for vnexr.
  Use when editing Gradle templates, build_android.sh, or device demo steps.
---

# XR Android Packaging

## Build / install

```bash
./scripts/build_android.sh -t Debug -a run -sample stereo_cube -abi arm64-v8a
```

There is a single APK, package `com.vertexnova.vnexr.samples` (the `vnexr_samples` launcher). The sample is chosen at launch: `-a run -sample <name>` passes the intent extra `sample`.

## Manifest checklist

- `android.software.xr.api.openxr` feature required
- Immersive HMD intent category: `org.khronos.openxr.intent.category.IMMERSIVE_HMD`
- Full-space start mode meta-data for unmanaged OpenXR activities
- OpenXR runtime broker `<queries>`, plus the `org.khronos.openxr.permission.OPENXR` and `OPENXR_SYSTEM` permissions (the loader is built from source, so no AAR merges them in)
- NativeActivity + `android.app.lib_name` pointing at the `vnexr_samples` launcher library
- Package the OpenXR loader `.so` built with the app (FetchContent / NDK link)
- Quest 3: `com.oculus.intent.category.VR` (otherwise Horizon OS starts a 2D panel), `com.oculus.permission.HAND_TRACKING` with optional `oculus.software.handtracking`, and `com.oculus.vr.focusaware`

Logs: `android_main` forwards stdout and stderr to logcat under the `vne.xr` tag, because NativeActivity drops them.

Templates live under `samples/framework/app/platform/android/gradle/`.
Generator: `cmake/VneXrAndroidGradle.cmake`.

## Device debug

```bash
adb devices
./scripts/verify_android_vulkan.sh
adb logcat -s vne.xr:* OpenXR:* vulkan:*
```

## Audience mirror

```bash
scrcpy --video-codec=h264 --max-size 1280
```

Record a clean run as fallback if the meeting room cannot mirror the headset.
