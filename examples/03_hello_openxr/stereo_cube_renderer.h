#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "vertexnova/xr/render_session.h"

namespace vne::xr_ns::examples {

/** @brief Minimal per-eye stereo cube hook (clears swapchain surfaces when available). */
class StereoCubeRenderer final : public IRenderSession {
   public:
    void on_session_ready() override;
    void update(const FrameParams& params, LayerParams& out_layers) override;

   private:
    std::uint64_t frames_rendered_ = 0;
};

}  // namespace vne::xr_ns::examples
