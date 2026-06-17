/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "common/logging_guard.h"
#include "vertexnova/xr/session.h"

namespace {

class GfxXrSceneApp final : public vne::xr_ns::IRenderSession {
   public:
    void on_session_ready() override {
        VNE_LOG_INFO << "vnegfx XR integration sample: render ECS scenes per XR view";
    }

    void update(const vne::xr_ns::FrameParams& params, vne::xr_ns::LayerParams& out_layers) override {
        (void)params;
        if (++frames_ >= 120) {
            out_layers.request_exit = true;
        }
    }

   private:
    std::uint64_t frames_ = 0;
};

}  // namespace

int main() {
    vne::xr_ns::examples::LoggingGuard logging_guard;

    vne::xr_ns::SessionConfig config;
    config.backend = vne::xr_ns::BackendType::eNull;
    config.application_name = "06_gfx_xr_scene";

    auto session = vne::xr_ns::create_session(config);
    GfxXrSceneApp app;
    session->run(app);
    return 0;
}
