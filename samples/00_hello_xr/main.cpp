/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "app/sample_launcher.h"
#include "framework/common/logging_guard.h"

namespace vne::xr::samples {
void registerHelloXrSample(SampleRegistry& registry);
}

int main() {
    vne::xr::samples::LoggingGuard guard;
    vne::xr::samples::SampleRegistry registry;
    vne::xr::samples::registerHelloXrSample(registry);
    return vne::xr::samples::launchSample(*registry.defaultSample());
}
