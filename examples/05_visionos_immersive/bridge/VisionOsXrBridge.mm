/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#import "VisionOsXrBridge.h"

#include "vertexnova/xr/session.h"

namespace {

class VisionOsImmersiveApp final : public vne::xr_ns::IRenderSession {
   public:
    void update(const vne::xr_ns::FrameParams& params, vne::xr_ns::LayerParams& out_layers) override {
        if (params.frame.frame_index >= 600) {
            out_layers.request_exit = true;
        }
    }
};

}  // namespace

void vnexr_visionos_run_immersive_session(void) {
    vne::xr_ns::SessionConfig config;
    config.backend = vne::xr_ns::BackendType::eVisionOs;
    config.application_name = "05_visionos_immersive";
    auto session = vne::xr_ns::create_session(config);
    VisionOsImmersiveApp app;
    session->run(app);
}
