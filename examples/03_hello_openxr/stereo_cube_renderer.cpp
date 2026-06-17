/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "stereo_cube_renderer.h"

#include "vertexnova/logging/logging.h"

#ifdef VNE_XR_WITH_RHI
#include "vertexnova/rhi/graphics_factory.h"
#endif

namespace vne::xr::examples {

void StereoCubeRenderer::onSessionReady() {
    VNE_LOG_INFO << "OpenXR stereo cube session ready";
#ifdef VNE_XR_WITH_RHI
    const auto backend = vne::rhi::GraphicsFactory::getDefaultBackend();
    (void)vne::rhi::GraphicsFactory::createPhysicalDevice(backend);
#endif
}

void StereoCubeRenderer::update(const FrameParams& params, LayerParams& out_layers) {
    if (!params.frame.should_render) {
        return;
    }
    for (std::uint32_t i = 0; i < params.frame.surfaces.view_count; ++i) {
        const auto& surface = params.frame.surfaces.views[i];
        VNE_LOG_INFO << "Stereo eye " << surface.view_index << " " << surface.width << "x"
                     << surface.height;
#ifdef VNE_XR_WITH_RHI
        if (surface.color_texture) {
            VNE_LOG_INFO << "  swapchain texture wrapped=" << surface.color_texture->isWrappedImage();
        }
#endif
    }
    ++frames_rendered_;
    if (frames_rendered_ >= 300) {
        out_layers.request_exit = true;
    }
}

}  // namespace vne::xr::examples
