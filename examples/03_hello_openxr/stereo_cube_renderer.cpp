/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   June 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

#include "stereo_cube_renderer.h"

#include "vertexnova/logging/logging.h"

#ifdef VNE_XR_WITH_RHI
#include "vertexnova/rhi/device.h"
#include "vertexnova/rhi/graphics_factory.h"
#endif

namespace vne::xr::examples {

void StereoCubeRenderer::onSessionReady() {
    VNE_LOG_INFO << "OpenXR stereo cube session ready";
    scene_.reset();
#ifdef VNE_XR_WITH_RHI
    const auto backend = vne::rhi::GraphicsFactory::getDefaultBackend();
    auto physical = vne::rhi::GraphicsFactory::createPhysicalDevice(backend);
    if (physical) {
        vne::rhi::PhysicalDeviceDescriptor phys_desc{};
        const auto info = physical->getDefaultDevice(phys_desc);
        vne::rhi::DeviceDescriptor dev_desc{};
        dev_desc.debug_name = "hello_openxr";
        device_ = physical->createDevice(info, dev_desc);
        if (device_) {
            VNE_LOG_INFO << "vnerhi device ready for per-eye XR surfaces";
        }
    }
#endif
}

void StereoCubeRenderer::clearEyeSurface(const ViewSurface& surface, float r, float g, float b) {
#ifdef VNE_XR_WITH_RHI
    if (!device_ || !surface.color_texture) {
        return;
    }
    VNE_LOG_INFO << "  clear eye " << surface.view_index << " " << surface.width << "x" << surface.height
                 << " rgba=(" << r << "," << g << "," << b << ")"
                 << " wrapped=" << surface.color_texture->isWrappedImage();
    if (surface.depth_texture) {
        VNE_LOG_INFO << "  depth surface available for eye " << surface.view_index;
    }
#else
    (void)surface;
    (void)r;
    (void)g;
    (void)b;
#endif
}

void StereoCubeRenderer::update(const FrameParams& params, LayerParams& out_layers) {
    if (!params.frame.should_render) {
        return;
    }

    scene_.update(params.input, 1.f / 72.f);

    const auto& blocks = scene_.blocks();
    float clear_r = 0.05f;
    float clear_g = 0.05f;
    float clear_b = 0.08f;
    if (blocks.size() > 1) {
        clear_r = blocks[1].color[0] * 0.2f;
        clear_g = blocks[1].color[1] * 0.2f;
        clear_b = blocks[1].color[2] * 0.2f;
    }

    for (std::uint32_t i = 0; i < params.frame.surfaces.view_count; ++i) {
        const auto& surface = params.frame.surfaces.views[i];
        const float eye_tint = (i == 0) ? 0.02f : -0.02f;
        clearEyeSurface(surface, clear_r + eye_tint, clear_g, clear_b + eye_tint);
    }

    if (params.frame.frame_index % 90 == 0) {
        VNE_LOG_INFO << "Stereo frame " << params.frame.frame_index << " blocks=" << blocks.size();
    }

    ++frames_rendered_;
    if (frames_rendered_ >= 3600) {
        out_layers.request_exit = true;
    }
}

}  // namespace vne::xr::examples
