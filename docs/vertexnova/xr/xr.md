# VneXR — XR session and compositor framework

## Overview

VneXR is the VertexNova **XR session/compositor framework**. It sits above **vnerhi** for GPU work and optionally consumes **vnegfx** for scene content — it does not duplicate gfx or RHI responsibilities.

| Platform | Compositor API | GPU (vnerhi) | App shell |
|----------|----------------|--------------|-----------|
| Android (Quest, Samsung Galaxy XR) | OpenXR + `XR_KHR_vulkan_enable` | Vulkan | NativeActivity / JNI |
| Windows (PC VR, simulators) | OpenXR | Vulkan or D3D12 | Win32 loop |
| visionOS | CompositorServices | Metal | SwiftUI + ObjC++ bridge |
| CI / headless | Null session | vnerhi Null | No runtime |

![System Context](diagrams/context.png)

## Stack position

```
Application (shell + XR content)
    └── vnexr (ISession, IFrame, IView, IRenderSession)
            ├── backend/openxr (Android, Windows)
            ├── backend/visionos (CompositorServices)
            ├── vnerhi (required) — swapchain → ITexture
            ├── vnemath (required) — poses, projections
            └── vnegfx (optional peer) — ECS scene content in XR views
```

## Core API (`vne::xr`)

| Type | Role |
|------|------|
| `ISession` | Lifecycle: poll events, begin/end frame |
| `IFrame` / `Frame` | Per-frame timing, views, surface textures |
| `IView` / `View` | Per-eye pose, FOV, view/projection matrices |
| `IRenderSession` | App callback: `update(FrameParams, LayerParams)` |
| `createSession(SessionConfig)` | Factory: Null, OpenXR, visionOS backends |

Portable render loop:

```cpp
#include "vertexnova/xr/session.h"

class MyApp : public vne::xr::IRenderSession {
  void update(const vne::xr::FrameParams& params,
              vne::xr::LayerParams& out) override {
    // render into params.frame.surfaces per eye
  }
};

vne::xr::SessionConfig cfg;
cfg.backend = vne::xr::BackendType::eOpenXr;
auto session = vne::xr::createSession(cfg);
MyApp app;
session->run(app);
```

## Module layout

```
include/vertexnova/xr/     Public headers
src/vertexnova/xr/
  core/                    Session state machine, Null backend
  backend/openxr/          OpenXR + Vulkan swapchain bridge
  backend/visionos/        CompositorServices C++ wrapper
cmake/                     VneXrDeps, VneXrOpenXR, VneXrVisionOS
examples/                  Platform samples (see examples/README.md)
```

## Dependency tiers

| Tier | Libraries | When |
|------|-----------|------|
| 0 | vnecmake, vnecommon, vnelogging, googletest | Always |
| 1 | **vnerhi**, **vnemath** | `VNE_XR_WITH_RHI=ON` (default) |
| 2 | Khronos OpenXR-SDK loader | `VNE_XR_WITH_OPENXR=ON` |
| 3 | CompositorServices, SwiftUI (visionOS SDK) | `VNE_XR_WITH_VISIONOS=ON` |
| 4 | vnegfx, entt (samples only) | `VNE_XR_WITH_GFX_SAMPLE=ON` |

**Do not vendor into vnexr core:** entt, imgui, Dawn/SPIRV toolchain, Assimp, vnescene sample libs.

Initialize deps:

```bash
export VNE_VNERHI_PATH=../vnerhi   # local private checkout
./scripts/init_submodules.sh full
```

## CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `VNE_XR_WITH_RHI` | ON | Link vnerhi + vnemath |
| `VNE_XR_WITH_OPENXR` | OFF (ON Android CI / Windows dev) | OpenXR backend |
| `VNE_XR_WITH_VISIONOS` | OFF (ON visionOS) | CompositorServices backend |
| `VNE_XR_WITH_GFX_SAMPLE` | OFF | Build vnegfx integration example |
| `VNE_XR_BACKEND_OPENXR_GRAPHICS` | vulkan | OpenXR graphics binding |
| `VNE_XR_TESTS` | ON (dev) | Unit tests |
| `VNE_XR_EXAMPLES` | ON (dev) | Example programs |
| `VNE_XR_LIB_TYPE` | shared | static or shared |

`VNE_XR_WITH_OPENXR` and `VNE_XR_WITH_VISIONOS` are mutually exclusive. OpenXR is blocked on Apple platforms.

CMake minimum: **3.26** (vnerhi requirement).

## Examples

| Example | Backend | Platform |
|---------|---------|----------|
| `01_hello_xr` | — | All |
| `02_null_session` | Null + vnerhi Null | CI / desktop |
| `03_hello_openxr` | OpenXR stereo cube | Android, Windows |
| `04_openxr_windows` | OpenXR + Win32 pump | Windows |
| `05_visionos_immersive` | CompositorServices | visionOS |
| `06_gfx_xr_scene` | Null + vnegfx | Optional sample |

Android build:

```bash
./scripts/build_android.sh --dev --with-examples
```

## Further reading

- [Coordinate conventions](conventions.md)
- [Diagrams](diagrams/README.md)
- Root [README.md](../../../README.md)
