# Linux setup (OpenXR + Vulkan)

Use a **Linux** machine (or VM) to build and run the OpenXR backend. macOS cannot compile OpenXR in vnexr (use visionOS path instead).

## Prerequisites

| Package | Purpose |
|---------|---------|
| GCC 10+ or Clang 10+ | C++20 |
| CMake 3.26+ | vnerhi requirement |
| `libvulkan-dev` | Vulkan + OpenXR graphics binding |
| X11 dev headers | Khronos OpenXR loader WSI on Linux |

Quick install (Debian/Ubuntu):

```bash
chmod +x scripts/*.sh
./scripts/install_linux_openxr_deps.sh
```

## Source checkout

```bash
git clone --recursive https://github.com/vertexnova/vnexr.git
cd vnexr

# Optional: local vnerhi checkout beside vnexr
export VNE_VNERHI_PATH=../vnerhi
./scripts/init_submodules.sh full
```

## Build

### Standard dev build (null backend, tests, samples)

```bash
./scripts/build_linux.sh -t Debug -a configure_and_build --dev
ctest --test-dir build/shared/Debug/build-linux-gcc-* --output-on-failure
```

### OpenXR build (Quest-style code path on desktop via Monado)

```bash
./scripts/build_linux.sh -t Debug -a configure_and_build --with-openxr --with-examples
```

Or with CMake directly:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug \
  -DVNE_XR_WITH_RHI=ON \
  -DVNE_XR_WITH_OPENXR=ON \
  -DVNE_XR_BACKEND_VULKAN=ON \
  -DVNE_XR_DEV=ON \
  -DVNE_XR_SAMPLES=ON
cmake --build build -j$(nproc)
```

`--dev` on Linux also defaults `VNE_XR_WITH_OPENXR=ON` (same as Windows dev builds).

## Run OpenXR sample

Install an OpenXR runtime (e.g. [Monado](https://monado.freedesktop.org/)):

```bash
# Example after Monado install
export XR_RUNTIME_JSON=/usr/share/openxr/1/openxr_monado.json   # path varies by distro
./build/shared/Debug/build-linux-gcc-*/bin/samples/sample_02_openxr_stereo
```

Without a runtime, session creation may fail at startup — that is expected on a headless CI machine; the **compile** is still validated.

## Platform notes

| Topic | Linux behavior |
|-------|----------------|
| Graphics API | Vulkan via `XR_KHR_vulkan_enable2` (fallback: `vulkan_enable`) |
| Getting-started sample | `01_null_simulator` (no runtime), then `02_openxr_stereo` |
| Win32-specific loop | Folded into `samples/framework/app/platform/desktop_loop` |
| Apple | OpenXR blocked — use `build_visionos.sh` |

## Troubleshooting

| Issue | Fix |
|-------|-----|
| `Could NOT find Vulkan` | Run `install_linux_openxr_deps.sh` or set `VULKAN_SDK` |
| `vnerhi not found` | `export VNE_VNERHI_PATH=...` and `init_submodules.sh full` |
| OpenXR instance fails | Install Monado; set `XR_RUNTIME_JSON` |
| No headset image | Need active OpenXR runtime + `session_running` state |

See also [openxr_design.md](openxr_design.md) and [conventions.md](conventions.md).
