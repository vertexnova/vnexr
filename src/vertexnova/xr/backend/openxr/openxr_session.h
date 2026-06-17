#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "vertexnova/xr/backend/openxr/openxr_swapchain_bridge.h"
#include "vertexnova/xr/session.h"

namespace vne::xr_ns {

/** @brief OpenXR session (Android / Windows). */
class OpenXrSession final : public ISession {
   public:
    explicit OpenXrSession(SessionConfig config);
    ~OpenXrSession() override;

    [[nodiscard]] BackendType backend_type() const override;
    [[nodiscard]] SessionState state() const override;
    bool poll_events() override;
    bool begin_frame(Frame& out_frame) override;
    void end_frame(const Frame& frame, const LayerParams& layers) override;

   private:
    SessionConfig config_;
    SessionState state_ = SessionState::eUnknown;
    bool running_ = true;
    void* instance_ = nullptr;
    void* session_ = nullptr;
    void* system_id_ = nullptr;
    void* reference_space_ = nullptr;
    std::uint64_t frame_index_ = 0;
    OpenXrSwapchainBridge swapchain_bridge_;
    std::uint32_t swapchain_width_ = 0;
    std::uint32_t swapchain_height_ = 0;
    bool swapchains_ready_ = false;

    bool init_openxr();
    void shutdown_openxr();
    bool ensure_swapchains(std::uint32_t view_count);
};

}  // namespace vne::xr_ns
