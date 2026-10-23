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

#include "vertexnova/xr/backend/visionos/visionos_session.h"

#include "vertexnova/rhi/graphics_factory.h"

#include <cstring>

namespace vne::xr {

namespace {

constexpr std::uint32_t kStubViewWidth = 1920;
constexpr std::uint32_t kStubViewHeight = 1080;

void setIdentityMat4(float* matrix) {
    std::memset(matrix, 0, sizeof(float) * kMat4FloatCount);
    constexpr std::uint32_t kDim = 4;
    for (std::uint32_t i = 0; i < kDim; ++i) {
        matrix[i * kDim + i] = 1.f;
    }
}

}  // namespace

VisionOsSession::VisionOsSession(SessionConfig config)
    : config_(std::move(config))
    , state_(SessionState::eReady) {
    const auto backend = vne::rhi::GraphicsFactory::getDefaultBackend();
    (void)vne::rhi::GraphicsFactory::createPhysicalDevice(backend);
}

VisionOsSession::~VisionOsSession() = default;

BackendType VisionOsSession::backendType() const {
    return BackendType::eVisionOs;
}

SessionState VisionOsSession::state() const {
    return state_;
}

bool VisionOsSession::pollEvents() {
    return running_;
}

bool VisionOsSession::beginFrame(Frame& out_frame) {
    if (!running_) {
        return false;
    }
    state_ = SessionState::eFocused;
    out_frame = {};
    out_frame.frame_index = frame_index_++;
    out_frame.should_render = true;
    out_frame.view_count = Frame::kMaxViews;
    out_frame.surfaces.view_count = Frame::kMaxViews;
    for (std::uint32_t i = 0; i < Frame::kMaxViews; ++i) {
        out_frame.views[i].view_index = i;
        out_frame.surfaces.views[i].view_index = i;
        out_frame.surfaces.views[i].width = kStubViewWidth;
        out_frame.surfaces.views[i].height = kStubViewHeight;
        setIdentityMat4(out_frame.views[i].view_matrix);
        setIdentityMat4(out_frame.views[i].projection_matrix);
    }
    return true;
}

void VisionOsSession::endFrame(const Frame& frame, const LayerParams& layers) {
    (void)frame;
    if (layers.request_exit) {
        running_ = false;
        state_ = SessionState::eExiting;
    }
}

}  // namespace vne::xr
