#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "vertexnova/xr/session.h"

namespace vne::xr_ns {

/** @brief Headless / CI session using vnerhi Null backend when available. */
class NullSession final : public ISession {
   public:
    explicit NullSession(SessionConfig config);
    ~NullSession() override;

    [[nodiscard]] BackendType backend_type() const override;
    [[nodiscard]] SessionState state() const override;
    bool poll_events() override;
    bool begin_frame(Frame& out_frame) override;
    void end_frame(const Frame& frame, const LayerParams& layers) override;

   private:
    SessionConfig config_;
    SessionState state_ = SessionState::eIdle;
    std::uint64_t frame_index_ = 0;
    bool running_ = true;
};

}  // namespace vne::xr_ns
