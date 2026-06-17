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

#include "vertexnova/xr/core/null_session.h"

#ifdef VNE_XR_WITH_RHI
#include "vertexnova/rhi/graphics_factory.h"
#endif

#include <cstring>

namespace vne::xr {

NullSession::NullSession(SessionConfig config)
    : config_(std::move(config)) {
    state_ = SessionState::eReady;
#ifdef VNE_XR_WITH_RHI
    const auto backend = vne::rhi::GraphicsFactory::getDefaultBackend();
    (void)vne::rhi::GraphicsFactory::createPhysicalDevice(backend);
#endif
}

NullSession::~NullSession() = default;

BackendType NullSession::backendType() const {
    return BackendType::eNull;
}

SessionState NullSession::state() const {
    return state_;
}

bool NullSession::pollEvents() {
    return running_;
}

bool NullSession::beginFrame(Frame& out_frame) {
    if (!running_) {
        return false;
    }
    state_ = SessionState::eFocused;
    out_frame = {};
    out_frame.frame_index = frame_index_++;
    out_frame.should_render = true;
    out_frame.view_count = 1;
    out_frame.views[0].view_index = 0;
    out_frame.surfaces.view_count = 1;
    out_frame.surfaces.views[0].view_index = 0;
    out_frame.surfaces.views[0].width = 1;
    out_frame.surfaces.views[0].height = 1;
    std::memset(out_frame.views[0].view_matrix, 0, sizeof(out_frame.views[0].view_matrix));
    out_frame.views[0].view_matrix[0] = out_frame.views[0].view_matrix[5] = out_frame.views[0].view_matrix[10] =
        out_frame.views[0].view_matrix[15] = 1.f;
    std::memset(out_frame.views[0].projection_matrix, 0, sizeof(out_frame.views[0].projection_matrix));
    out_frame.views[0].projection_matrix[0] = out_frame.views[0].projection_matrix[5] =
        out_frame.views[0].projection_matrix[10] = out_frame.views[0].projection_matrix[15] = 1.f;
    return true;
}

void NullSession::endFrame(const Frame& frame, const LayerParams& layers) {
    (void)frame;
    if (layers.request_exit) {
        running_ = false;
        state_ = SessionState::eExiting;
    }
}

}  // namespace vne::xr
