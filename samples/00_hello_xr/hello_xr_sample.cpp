/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

/**
 * @file hello_xr_sample.cpp
 * @brief Sample hello_xr -- library smoke test: prints the version and runs one null-session frame.
 */

#include "app/sample_registry.h"
#include "app/xr_sample_app.h"
#include "framework/common/logging_guard.h"
#include "layers/exit_after_frames_layer.h"
#include "vertexnova/xr/xr.h"

#include <memory>

namespace vne::xr::samples {

void registerHelloXrSample(SampleRegistry& registry) {
    registry.add({
        .name = "hello_xr",
        .description = "Library smoke test: prints the version and runs one frame (no headset needed)",
        .backend = BackendType::eNull,
        .setup =
            [](XrSampleApp& app) {
                VNE_LOG_INFO << vne::xr::hello();
                VNE_LOG_INFO << "Version: " << vne::xr::getVersion();
                app.pushLayer(std::make_unique<ExitAfterFramesLayer>(1));
            },
    });
}

}  // namespace vne::xr::samples
