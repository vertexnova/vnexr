#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   June 2026
 *
 * Autodoc:   yes
 * ---------------------------------------------------------------------- */

/**
 * @file swapchain_image.h
 * @brief Compositor swapchain surfaces bridged to vne::rhi::ITexture.
 */

#include "vertexnova/xr/types.h"
#include "vertexnova/rhi/render_target.h"
#include "vertexnova/rhi/texture.h"

#include <cstdint>
#include <memory>

namespace vne::xr {

/**
 * @brief Per-eye or per-view targets for one render pass.
 *
 * @details `render_target` is owned by the session and valid until the next `beginFrame()`; it already combines the
 * (possibly multisampled) color and depth attachments, with the compositor image as the resolve target.
 * It is null on backends that render nothing (Null session).
 */
struct ViewSurface {
    std::uint32_t view_index = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    vne::rhi::IRenderTarget* render_target = nullptr;
    vne::rhi::TextureFormat color_format = vne::rhi::TextureFormat::eUnknown;
    vne::rhi::TextureFormat depth_format = vne::rhi::TextureFormat::eUnknown;
    vne::rhi::SampleCount sample_count = vne::rhi::SampleCount::e1;
    /** @brief Increases whenever the session recreates its swapchains; pipelines built for old targets are stale. */
    std::uint32_t swapchain_generation = 0;
    /** @brief The compositor image (1x), for code that needs the texture itself. */
    std::shared_ptr<vne::rhi::ITexture> color_texture;
};

/** @brief All swapchain-backed surfaces for the current frame. */
struct SurfaceTextures {
    std::uint32_t view_count = 0;
    static constexpr std::uint32_t kMaxViews = 2;
    ViewSurface views[kMaxViews];
};

}  // namespace vne::xr
