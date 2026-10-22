---
name: vne-xr-session
description: >
  Host-owned XR graphics device and swapchain rules for vnexr. Use when editing
  OpenXR session code, swapchain bridges, or sample layers that draw into Frame surfaces.
---

# XR Session Module Conventions

## Ownership

1. The **host session** (`OpenXrSession`) creates the Vulkan device from OpenXR
   requirements (`XR_KHR_vulkan_enable2` hooks into vnerhi), creates the XR
   session, and owns color/depth swapchains.
2. Samples call `ISession::graphicsDevice()` and draw only into
   `Frame.surfaces.views[i].color_texture` / `depth_texture`.
3. Samples must **not** create a second `IDevice`, call `xrCreateSession`, or
   invent their own Vulkan instance for compositor images.

## Frame loop

```
pollEvents -> beginFrame (fills Frame + SurfaceTextures)
  -> sample IXrLayer::onUpdate(FrameParams, LayerParams)
  -> endFrame (composition layers)
```

Host owns wait/begin/end frame and layer submission. The sample only renders.

## Input

Suggest controller profiles (KHR simple, Oculus Touch) and
`/interaction_profiles/ext/hand_interaction_ext` when `XR_EXT_hand_interaction`
is enabled. Pinch maps to spawn; grasp maps to grab.

## Checklist before merging

- [ ] App uses `session->graphicsDevice()`, not a factory-created device for XR draws
- [ ] Textures come from the swapchain bridge wrap path
- [ ] No `#ifdef VNE_XR_WITH_RHI` (RHI is a hard dependency)
