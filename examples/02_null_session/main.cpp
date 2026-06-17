/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "common/logging_guard.h"
#include "vertexnova/xr/session.h"

namespace {

class NullRenderSession final : public vne::xr::IRenderSession {
   public:
    void update(const vne::xr::FrameParams& params, vne::xr::LayerParams& out_layers) override {
        VNE_LOG_INFO << "Null XR frame " << params.frame.frame_index << " views="
                     << params.frame.view_count;
        if (params.frame.frame_index >= 2) {
            out_layers.request_exit = true;
        }
    }
};

}  // namespace

int main() {
    vne::xr::examples::LoggingGuard logging_guard;

    vne::xr::SessionConfig config;
    config.backend = vne::xr::BackendType::eNull;
    config.application_name = "02_null_session";

    auto session = vne::xr::createSession(config);
    NullRenderSession app;
    session->run(app);
    return 0;
}
