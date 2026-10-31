#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "vertexnova/xr/frame.h"
#include "vertexnova/xr/render_session.h"
#include "vertexnova/xr/session.h"

#include "common/xr_math.h"

#include "vertexnova/rhi/device.h"
#include "vertexnova/rhi/render_command_encoder.h"

#include <string>

namespace vne::xr::samples {

/**
 * @brief Everything a layer needs to draw one eye into an already open render pass.
 *
 * @details The framework owns the render target, command buffer and submit; a layer only issues draw calls on
 * `encoder`. `view_proj` is projection * view * world_from_scene, so a layer works in its own scene space and
 * never needs to know about navigation. Pipelines must match `color_format`, `depth_format` and `sample_count`:
 * describe them with `setEyeTargetFormats()` (render/eye_pipeline.h), which also sets the rasterizer sample count
 * that vnerhi actually uses. They are stale once `swapchain_generation` changes.
 */
struct EyeRenderContext {
    std::uint32_t eye_index = 0;
    const View* view = nullptr;
    xrmath::Mat4 view_proj = xrmath::identity();
    xrmath::Mat4 world_from_scene = xrmath::identity();
    vne::rhi::IRenderCommandEncoder* encoder = nullptr;
    vne::rhi::TextureFormat color_format = vne::rhi::TextureFormat::eUnknown;
    vne::rhi::TextureFormat depth_format = vne::rhi::TextureFormat::eUnknown;
    vne::rhi::SampleCount sample_count = vne::rhi::SampleCount::e1;
    std::uint32_t swapchain_generation = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

/**
 * @brief Base class for XR sample layers (scene, input, debug overlays).
 *
 * Mirrors vnegfx/vnerhi ILayer but driven by vne::xr::FrameParams each XR frame.
 */
class IXrLayer {
   public:
    explicit IXrLayer(std::string name = "XrLayer");
    virtual ~IXrLayer() = default;

    IXrLayer(const IXrLayer&) = delete;
    IXrLayer& operator=(const IXrLayer&) = delete;

    /** @brief One-time setup after the session device exists. */
    virtual void onAttach(ISession* session) { (void)session; }
    virtual void onDetach() {}

    /** Called once per XR frame before per-eye rendering hooks. */
    virtual void onUpdate(const FrameParams& params, LayerParams& out_layers) {
        (void)params;
        (void)out_layers;
    }

    /**
     * @brief Draw this layer into the open render pass of one eye.
     * @details Called after `onUpdate()`, once per eye, only for frames the compositor will display.
     */
    virtual void onRenderEye(const EyeRenderContext& context) { (void)context; }

    void setEnabled(bool enabled) { enabled_ = enabled; }
    [[nodiscard]] bool isEnabled() const { return enabled_; }

    [[nodiscard]] const std::string& name() const { return name_; }

   private:
    bool enabled_ = true;
    std::string name_;
};

}  // namespace vne::xr::samples
