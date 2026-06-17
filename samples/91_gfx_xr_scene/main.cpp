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

#include "framework/common/logging_guard.h"
#include "vertexnova/xr/session.h"

namespace {

class GfxXrSceneApp final : public vne::xr::IRenderSession {
   public:
    void onSessionReady() override { VNE_LOG_INFO << "vnegfx XR integration sample: render ECS scenes per XR view"; }

    void update(const vne::xr::FrameParams& params, vne::xr::LayerParams& out_layers) override {
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
    vne::xr::examples::LoggingGuard logging_guard;

    vne::xr::SessionConfig config;
    config.backend = vne::xr::BackendType::eNull;
    config.application_name = "06_gfx_xr_scene";

    auto session = vne::xr::createSession(config);
    GfxXrSceneApp app;
    session->run(app);
    return 0;
}
