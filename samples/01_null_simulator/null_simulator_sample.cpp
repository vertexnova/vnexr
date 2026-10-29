/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

/**
 * @file null_simulator_sample.cpp
 * @brief Sample null_simulator -- null XR backend with the layer stack (no OpenXR runtime).
 */

#include "app/sample_registry.h"
#include "app/xr_sample_app.h"
#include "layers/exit_after_frames_layer.h"

#include <memory>

namespace vne::xr::samples {

void registerNullSimulatorSample(SampleRegistry& registry) {
    registry.add({
        .name = "null_simulator",
        .description = "Null session and layer stack: five simulated frames, no headset or runtime",
        .backend = BackendType::eNull,
        .setup = [](XrSampleApp& app) { app.pushLayer(std::make_unique<ExitAfterFramesLayer>(5)); },
    });
}

}  // namespace vne::xr::samples
