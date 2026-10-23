/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

/**
 * @file main.cpp
 * @brief Sample 03 — OpenXR + vnegfx CSG on the session-owned device (stretch).
 */

#include "csg_xr_scene_layer.h"
#include "app/xr_sample_app.h"
#include "framework/common/logging_guard.h"
#include "vertexnova/xr/session.h"

int main() {
    vne::xr::samples::LoggingGuard guard;

    vne::xr::SessionConfig config;
    config.backend = vne::xr::BackendType::eOpenXr;
    config.application_name = "03_openxr_csg";

    vne::xr::samples::XrSampleApp app(config);
    app.pushLayer(std::make_unique<vne::xr::samples::CsgXrSceneLayer>());
    return app.run();
}
