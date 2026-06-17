#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "vertexnova/xr/types.h"

#ifdef VNE_XR_WITH_RHI
#include "vertexnova/rhi/texture.h"
#endif

#include <cstdint>
#include <memory>

namespace vne::xr_ns {

/** @brief Per-eye or per-view color/depth targets for one render pass. */
struct ViewSurface {
    std::uint32_t view_index = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
#ifdef VNE_XR_WITH_RHI
    std::shared_ptr<vne::rhi::ITexture> color_texture;
    std::shared_ptr<vne::rhi::ITexture> depth_texture;
#endif
};

/** @brief All swapchain-backed surfaces for the current frame. */
struct SurfaceTextures {
    std::uint32_t view_count = 0;
    static constexpr std::uint32_t kMaxViews = 2;
    ViewSurface views[kMaxViews]{};
};

}  // namespace vne::xr_ns
