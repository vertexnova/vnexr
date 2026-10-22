/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "csg_xr_scene_layer.h"

#include "vertexnova/logging/logging.h"

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.xr.samples.openxr_csg");
}

namespace vne::xr::samples {

CsgXrSceneLayer::CsgXrSceneLayer()
    : IXrLayer("CsgXrScene") {}

CsgXrSceneLayer::~CsgXrSceneLayer() {
    onDetach();
}

void CsgXrSceneLayer::onAttach(ISession* session) {
    if (session == nullptr) {
        return;
    }
    device_ = session->graphicsDevice();
    if (!device_) {
        VNE_LOG_ERROR << "CSG XR sample: session has no graphics device";
        return;
    }
    // GfxSession adopts the host device (never create a second Vulkan device).
    constexpr std::uint32_t kBootstrapWidth = 1280;
    constexpr std::uint32_t kBootstrapHeight = 720;
    gfx_session_ = std::make_unique<vne::gfx::GfxSession>(*device_, kBootstrapWidth, kBootstrapHeight);
    VNE_LOG_INFO << "CSG XR sample: GfxSession attached to session device";
}

void CsgXrSceneLayer::onDetach() {
    gfx_session_.reset();
    device_.reset();
}

void CsgXrSceneLayer::onUpdate(const FrameParams& params, LayerParams& out_layers) {
    if (!params.frame.should_render || !gfx_session_ || !device_) {
        return;
    }

    if (params.frame.surfaces.view_count == 0) {
        if (!warned_missing_surfaces_) {
            VNE_LOG_WARN << "CSG XR sample: no surface textures yet";
            warned_missing_surfaces_ = true;
        }
        return;
    }

    // No rendering yet: do not present acquired color/depth images.
    // When CSG draw is wired, set submit_projection = true after writing eyes.
    out_layers.submit_projection = false;
    ++frames_;
    if (frames_ == 1) {
        VNE_LOG_INFO << "CSG XR sample: frame loop live (zero projection layers); "
                        "stereo CSG draw is future work";
    }
    if (frames_ >= 3600) {
        out_layers.request_exit = true;
    }
}

}  // namespace vne::xr::samples
