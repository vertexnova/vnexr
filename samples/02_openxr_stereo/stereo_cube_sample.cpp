/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

/**
 * @file stereo_cube_sample.cpp
 * @brief Sample stereo_cube -- OpenXR stereo cubes with grab/spawn interaction (Monado / Quest / PC VR).
 */

#include "stereo_cube_scene_layer.h"

#include "app/sample_registry.h"
#include "app/xr_sample_app.h"

#include <memory>

namespace vne::xr::samples {

void registerStereoCubeSample(SampleRegistry& registry) {
    registry.add({
        .name = "stereo_cube",
        .description = "OpenXR stereo cubes with controller and hand grab/spawn interaction",
        .backend = BackendType::eOpenXr,
        .setup = [](XrSampleApp& app) { app.pushLayer(std::make_unique<StereoCubeSceneLayer>()); },
    });
}

}  // namespace vne::xr::samples
