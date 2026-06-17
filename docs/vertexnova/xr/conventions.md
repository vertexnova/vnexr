# VneXR coordinate and rendering conventions

Backends may differ in handedness, depth range, and NDC Z. Applications should use matrices supplied in `Frame::views[]` rather than assuming a single global convention.

## Shared contract (`vne::xr`)

| Field | Convention |
|-------|------------|
| `Pose.position` | Meters, runtime world space |
| `Pose.orientation` | Quaternion `(x, y, z, w)` |
| `FieldOfView` | Radians: left/right/up/down from center |
| `View.view_matrix` | Row-major 4×4, world → eye |
| `View.projection_matrix` | Row-major 4×4, eye → clip |
| `LayerParams.near_z` / `far_z` | App hint for depth submission |

## OpenXR (Android / Windows)

| Topic | Convention |
|-------|------------|
| Reference space | `XR_REFERENCE_SPACE_TYPE_LOCAL` at session start |
| Graphics | Vulkan via `XR_KHR_vulkan_enable2` (preferred) or `XR_KHR_vulkan_enable` |
| Swapchain images | Per-eye color + depth; compositor-owned; wrapped as `vne::rhi::ITexture` |
| Depth | Depth swapchain for rendering; composition layer depth deferred |
| Module layout | See [openxr_design.md](openxr_design.md) |
| Samsung Galaxy XR | Same path as Quest: Android + OpenXR + Vulkan |

**Rendering:** reverse-Z is recommended when matching vnerhi Vulkan defaults; confirm against your shader pipeline.

## visionOS (CompositorServices)

| Topic | Convention |
|-------|------------|
| Shell | SwiftUI `ImmersiveSpace` + `CompositorLayer` |
| Bridge | ObjC++ `VisionOsXrBridge` → C++ `VisionOsSession` |
| GPU | Metal via vnerhi Metal backend |
| Drawables | Per-frame `ViewProj` from compositor; no window `Present()` |
| Preview | Optional `CAMetalLayer` 2D window sharing device |

**Note:** OpenXR is not used on Apple platforms.

## Null backend (CI)

Single view, 1×1 surfaces, identity matrices. Used for headless tests and `02_null_session`.

## vnegfx integration (optional)

vnegfx remains a peer library. vnexr provides viewports and frame timing; the sample (`06_gfx_xr_scene`) renders ECS content into each `ViewSurface` without pulling gfx into core.
