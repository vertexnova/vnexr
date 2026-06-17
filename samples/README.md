# VneXR Samples

Sample programs for the VertexNova XR library, structured like **vnerhi** and **vnegfx** (`samples/` + shared framework).

## Build

```bash
# macOS — null backend only (samples 00–01)
cmake -B build -DVNE_XR_DEV=ON -DVNE_XR_LIB_TYPE=shared
cmake --build build

# Linux — OpenXR + Monado simulator
./scripts/install_linux_openxr_deps.sh
./scripts/build_linux.sh -t Debug -a configure_and_build --with-openxr --with-examples
```

`--with-examples` is a script alias for `--with-samples`; both set `VNE_XR_SAMPLES=ON`.

Binaries land in `build/<lib_type>/<config>/bin/samples/`.

## Getting started path

| Sample | Backend | What you learn |
|--------|---------|----------------|
| **00_hello_xr** | — | Link `vne::xr`, call `hello()` |
| **01_null_simulator** | Null | `XrSampleApp` + `IXrLayer` stack without a headset (CI / headless) |
| **02_openxr_stereo** | OpenXR | Stereo rendering + block grab/spawn on Monado, PC VR, or Quest |

Start with **01** on any host, then **02** once you have an OpenXR runtime.

### Run (null simulator — no runtime)

```bash
./build/shared/Debug/bin/samples/sample_01_null_simulator
```

### Run (OpenXR + Monado on Linux)

```bash
export XR_RUNTIME_JSON=/usr/share/openxr/1/openxr_monado.json
./build/shared/Debug/build-linux-gcc-*/bin/samples/sample_02_openxr_stereo
```

## Layer architecture

Samples use a thin **layer stack** (analogous to vnegfx `ILayer`):

```
main.cpp
  └─ XrSampleApp (owns ISession, implements IRenderSession)
       └─ XrLayerStack
            ├─ Scene layers   (e.g. StereoCubeSceneLayer)
            └─ Overlay layers (optional, drawn last)
```

Each frame the session calls `XrSampleApp::update()` → `XrLayerStack::onUpdate(FrameParams, LayerParams)`.

Add a new sample by:

1. Subclass `IXrLayer` in your sample folder.
2. `app.pushLayer(std::make_unique<MyLayer>())`.
3. `return app.run()` (desktop) or wire `android_main` (Android).

Framework lives in `samples/framework/` (`vne::xr::samples::framework`).

## Android setup (sample 02)

OpenXR on Android requires loader init **before** `createSession`. Set platform handles in `SessionConfig`; the OpenXR backend calls `xrInitializeLoaderKHR` internally:

```cpp
config.platform.application_vm = android_app->activity->vm;
config.platform.application_context = android_app->activity->clazz;
```

Build with the Android script:

```bash
./scripts/build_android.sh -t Debug -a configure_and_build --with-openxr --with-examples
```

Link `android`, `log`, and `native_app_glue` (see `02_openxr_stereo/CMakeLists.txt`). Entry point is `android_main`, not `main`.

## Optional samples

| Sample | Requires |
|--------|----------|
| **90_visionos_immersive** | `VNE_XR_WITH_VISIONOS`, visionOS target |
| **91_gfx_xr_scene** | `VNE_XR_WITH_GFX_SAMPLE`, `vnegfx` |

## Platform notes

- **macOS**: OpenXR is not supported; build samples 00–01 only.
- **Linux / Windows**: Use sample 02 with Monado, SteamVR, or Windows Mixed Reality.
- **Android**: Quest / Pico via OpenXR loader + platform handles above.
