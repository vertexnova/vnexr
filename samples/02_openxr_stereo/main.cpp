/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

/**
 * @file main.cpp
 * @brief Standalone desktop entry for the stereo_cube sample. On Android the launcher runs it by name.
 */

#include "app/sample_launcher.h"
#include "framework/common/logging_guard.h"

namespace vne::xr::samples {
void registerStereoCubeSample(SampleRegistry& registry);
}

int main() {
    vne::xr::samples::LoggingGuard guard;
    vne::xr::samples::SampleRegistry registry;
    vne::xr::samples::registerStereoCubeSample(registry);
    return vne::xr::samples::launchSample(*registry.defaultSample());
}
