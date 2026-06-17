/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "vertexnova/xr/backend/visionos/visionos_session.h"

#include <cstring>

#ifdef VNE_XR_WITH_RHI
#include "vertexnova/rhi/graphics_factory.h"
#endif

namespace vne::xr_ns {

VisionOsSession::VisionOsSession(SessionConfig config)
    : config_(std::move(config)) {
    state_ = SessionState::eReady;
#ifdef VNE_XR_WITH_RHI
    const auto backend = vne::rhi::GraphicsFactory::getDefaultBackend();
    (void)vne::rhi::GraphicsFactory::createPhysicalDevice(backend);
#endif
}

VisionOsSession::~VisionOsSession() = default;

BackendType VisionOsSession::backend_type() const {
    return BackendType::eVisionOs;
}

SessionState VisionOsSession::state() const {
    return state_;
}

bool VisionOsSession::poll_events() {
    return running_;
}

bool VisionOsSession::begin_frame(Frame& out_frame) {
    if (!running_) {
        return false;
    }
    state_ = SessionState::eFocused;
    out_frame = {};
    out_frame.frame_index = frame_index_++;
    out_frame.should_render = true;
    out_frame.view_count = 2;
    out_frame.surfaces.view_count = 2;
    for (std::uint32_t i = 0; i < 2; ++i) {
        out_frame.views[i].view_index = i;
        out_frame.surfaces.views[i].view_index = i;
        out_frame.surfaces.views[i].width = 1920;
        out_frame.surfaces.views[i].height = 1080;
        std::memset(out_frame.views[i].view_matrix, 0, sizeof(out_frame.views[i].view_matrix));
        out_frame.views[i].view_matrix[0] = out_frame.views[i].view_matrix[5] = out_frame.views[i].view_matrix[10] =
            out_frame.views[i].view_matrix[15] = 1.f;
        std::memset(out_frame.views[i].projection_matrix, 0, sizeof(out_frame.views[i].projection_matrix));
        out_frame.views[i].projection_matrix[0] = out_frame.views[i].projection_matrix[5] =
            out_frame.views[i].projection_matrix[10] = out_frame.views[i].projection_matrix[15] = 1.f;
    }
    return true;
}

void VisionOsSession::end_frame(const Frame& frame, const LayerParams& layers) {
    (void)frame;
    if (layers.request_exit) {
        running_ = false;
        state_ = SessionState::eExiting;
    }
#ifdef VNE_XR_WITH_RHI
    // Compositor-owned drawables: flush GPU work without window Present (Diligent T30 pattern).
    (void)layers;
#endif
}

}  // namespace vne::xr_ns
