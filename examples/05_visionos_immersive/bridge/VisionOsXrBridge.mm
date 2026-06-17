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

#import "VisionOsXrBridge.h"

#include "vertexnova/xr/session.h"

namespace {

class VisionOsImmersiveApp final : public vne::xr::IRenderSession {
   public:
    void update(const vne::xr::FrameParams& params, vne::xr::LayerParams& out_layers) override {
        if (params.frame.frame_index >= 600) {
            out_layers.request_exit = true;
        }
    }
};

}  // namespace

void vnexr_visionos_run_immersive_session(void) {
    vne::xr::SessionConfig config;
    config.backend = vne::xr::BackendType::eVisionOs;
    config.application_name = "05_visionos_immersive";
    auto session = vne::xr::createSession(config);
    VisionOsImmersiveApp app;
    session->run(app);
}
