/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "app/sample_launcher.h"

#include "framework/common/logging_guard.h"

#include <utility>

namespace vne::xr::samples {

int launchSample(const SampleInfo& sample, const SessionPlatformHandle& platform, XrSampleApp::FramePump pump) {
    VNE_LOG_INFO << "Running sample '" << sample.name << "': " << sample.description;

    SessionConfig config;
    config.backend = sample.backend;
    config.application_name = sample.name;
    config.platform = platform;

    XrSampleApp app(std::move(config));
    sample.setup(app.stack());
    app.setFramePump(std::move(pump));
    return app.run();
}

}  // namespace vne::xr::samples
