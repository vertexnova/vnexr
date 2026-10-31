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
| **02_openxr_stereo** | OpenXR | Stereo rendering + block grab/spawn on Monado, PC VR, or Galaxy XR |
| **03_openxr_csg** | OpenXR + vnegfx | Stretch: CSG on the session-owned device (Monado first) |

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

## One launcher for every sample

`vnexr_samples` is a single binary that runs any registered sample by name. Each sample also keeps its own standalone executable on desktop.

| Sample name | Directory | Backend |
|-------------|-----------|---------|
| `hello_xr` | `00_hello_xr` | Null |
| `null_simulator` | `01_null_simulator` | Null |
| `stereo_cube` | `02_openxr_stereo` | OpenXR |
| `xr_diagnostics` | `04_xr_diagnostics` | OpenXR |

```bash
vnexr_samples --list
vnexr_samples --sample null_simulator
```

On Android the same library is the only NativeActivity and one `XrSampleHost` session runs for the whole app. With no launch extra the in-headset launcher opens; the left Menu button returns to it from any sample. To jump into a sample, use the launch intent: `adb shell am start -n com.vertexnova.vnexr.samples/com.vertexnova.vnexr.samples.VneXrNativeActivity -e sample stereo_cube`, or `./scripts/build_android.sh -a run -sample stereo_cube`. A second start while the app is running switches the sample in place. On a desktop OpenXR runtime (for example Monado) `vnexr_samples --launcher` opens the same launcher.

A sample is a stack of layers (`SampleInfo::setup` receives an `XrLayerStack`). Override `onUpdate` for logic and `onRenderEye` to draw: the framework owns the render target, command buffer and submit, draws both eyes every frame, and gives the layer input and drawing already expressed in its own scene coordinates, so grip-to-move/zoom works in every sample with no sample code. `FlatColorRenderer` draws shaded boxes and meshes.

### Adding a sample

1. In the sample's `CMakeLists.txt` call `vne_xr_add_sample_library(<name> REGISTER register<Name>Sample SOURCES ... [LIBRARIES ...])`.
2. In its sources define `void vne::xr::samples::register<Name>Sample(SampleRegistry&)`, which calls `registry.add` with an explicit initializer for each field. `setup` receives the sample's `XrLayerStack` and pushes its layers onto it.

   ```cpp
   registry.add({
       .name = "my_sample",
       .description = "What the sample shows",
       .backend = BackendType::eOpenXr,
       .setup = [](XrLayerStack& stack) { stack.pushLayer(std::make_unique<MyLayer>()); },
   });
   ```

3. The launcher picks it up on the next configure: its registration list is generated from the `vne_xr_add_sample_library` calls, so nothing else needs editing.

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

On Android only the `vnexr_samples` launcher is a NativeActivity library (it links `android`, `log` and `native_app_glue`, entry point `android_main`); every sample is a static library with a `register<Name>Sample()` function, see `launcher/CMakeLists.txt`.

## Optional samples

| Sample | Requires |
|--------|----------|
| **90_visionos_immersive** | `VNE_XR_WITH_VISIONOS`, visionOS target |
| **91_gfx_xr_scene** | `VNE_XR_WITH_GFX_SAMPLE`, `vnegfx` |

## Platform notes

- **macOS**: OpenXR is not supported; build samples 00–01 only.
- **Linux / Windows**: Use sample 02 with Monado, SteamVR, or Windows Mixed Reality.
- **Android**: Quest / Pico via OpenXR loader + platform handles above.
