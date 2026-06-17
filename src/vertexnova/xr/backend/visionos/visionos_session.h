#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "vertexnova/xr/session.h"

namespace vne::xr {

/**
 * @brief visionOS CompositorServices session (Metal via vnerhi).
 *
 * Full CompositorLayer / cp_layer_renderer integration lives in the platform shell;
 * this C++ session drives the portable render loop contract.
 */
class VisionOsSession final : public ISession {
   public:
    explicit VisionOsSession(SessionConfig config);
    ~VisionOsSession() override;

    [[nodiscard]] BackendType backendType() const override;
    [[nodiscard]] SessionState state() const override;
    bool pollEvents() override;
    bool beginFrame(Frame& out_frame) override;
    void endFrame(const Frame& frame, const LayerParams& layers) override;

   private:
    SessionConfig config_;
    SessionState state_ = SessionState::eIdle;
    bool running_ = true;
    std::uint64_t frame_index_ = 0;
};

}  // namespace vne::xr
