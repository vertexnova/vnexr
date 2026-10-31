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
        .description = "OpenXR stereo cubes grab and spawn blocks with the trigger, A/X recolors",
        .backend = BackendType::eOpenXr,
        .setup = [](XrLayerStack& app) { app.pushLayer(std::make_unique<StereoCubeSceneLayer>()); },
        .accent_color = {1.00f, 0.55f, 0.20f},
    });
}

}  // namespace vne::xr::samples
