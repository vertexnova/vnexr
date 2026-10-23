# Sample platforms

| Sample | Ubuntu 24.04 | Windows | Android (Galaxy XR) | macOS |
|--------|--------------|---------|---------------------|-------|
| `00_hello_xr` | yes | yes | NDK link | yes |
| `01_null_simulator` | yes | yes | NDK | yes |
| `02_openxr_stereo` | Monado | OpenXR runtime | APK arm64 | no (OpenXR blocked) |
| `03_openxr_csg` | Monado + gfx | optional | no | no |
| `90_visionos_immersive` | no | no | no | visionOS |
| `91_gfx_xr_scene` | Null + gfx | optional | no | Null + gfx |

## Ubuntu OpenXR

```bash
./scripts/install_linux_openxr_deps.sh
./scripts/build_linux.sh -t Debug -a configure_and_build --with-openxr --with-examples
export XR_RUNTIME_JSON=/usr/share/openxr/1/openxr_monado.json
./build/shared/Debug/build-linux-gcc-*/bin/samples/sample_02_openxr_stereo
```

## Android / Galaxy XR

```bash
./scripts/build_android.sh -t Debug -a install \
  -sample sample_02_openxr_stereo -abi arm64-v8a
./scripts/verify_android_vulkan.sh com.vertexnova.vnexr.openxr_stereo
```

Gradle project lands in the CMake build tree under `android-studio/`.

## Audience mirror

```bash
scrcpy --video-codec=h264 --max-size 1280
```
