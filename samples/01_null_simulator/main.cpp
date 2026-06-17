/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

/**
 * @file main.cpp
 * @brief Sample 01 — null XR backend (simulator / CI, no OpenXR runtime).
 *
 * Demonstrates XrSampleApp + layer stack without a headset or Monado.
 */

#include "exit_after_frames_layer.h"
#include "app/xr_sample_app.h"
#include "framework/common/logging_guard.h"
#include "vertexnova/xr/session.h"

#include <memory>

int main() {
    vne::xr::samples::LoggingGuard guard;

    vne::xr::SessionConfig config;
    config.backend = vne::xr::BackendType::eNull;
    config.application_name = "01_null_simulator";

    vne::xr::samples::XrSampleApp app(config);
    app.pushLayer(std::make_unique<vne::xr::samples::ExitAfterFramesLayer>(5));
    return app.run();
}
