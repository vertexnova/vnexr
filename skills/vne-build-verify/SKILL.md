---
name: vne-build-verify
description: >
  Standard format, build, run, and test pipeline for vnexr on Ubuntu 24.04
  (OpenXR + Monado) and Android (Galaxy XR APK). Use before finishing a change.
---

# Build and Verify (vnexr)

Primary host: **Ubuntu 24.04**. macOS builds Null/visionOS only (OpenXR is blocked).

## 1. Format

```bash
python3 scripts/clang_formatter.py all
python3 scripts/clang_formatter.py all --dry-run
```

## 2. Dependencies

```bash
export VNE_VNERHI_PATH=../vnerhi   # optional local checkout
./scripts/init_submodules.sh full
./scripts/install_linux_openxr_deps.sh
```

## 3. Desktop OpenXR (Monado backup demo)

```bash
./scripts/build_linux.sh -t Debug -a configure_and_build --with-openxr --with-examples
ctest --test-dir build/shared/Debug/build-linux-gcc-* --output-on-failure

systemctl --user start monado.service   # or monado-service &
export XR_RUNTIME_JSON=/usr/share/openxr/1/openxr_monado.json
./build/shared/Debug/build-linux-gcc-*/bin/samples/sample_01_null_simulator
./build/shared/Debug/build-linux-gcc-*/bin/samples/sample_02_openxr_stereo
```

## 4. Android / Galaxy XR

```bash
export ANDROID_HOME=$HOME/Android/Sdk
export ANDROID_NDK_ROOT=$ANDROID_HOME/ndk/<version>

./scripts/build_android.sh -t Debug -a install \
  -sample sample_02_openxr_stereo -abi arm64-v8a

./scripts/verify_android_vulkan.sh com.vertexnova.vnexr.openxr_stereo
adb logcat -s vne.xr:* OpenXR:*
```

## 5. Done when

- Null sample runs without a runtime.
- `02_openxr_stereo` runs on Monado (desktop) or Galaxy XR (APK).
- No new non-ASCII in `skills/` or edited docs.
