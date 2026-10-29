#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "app/sample_registry.h"
#include "app/xr_sample_app.h"
#include "vertexnova/xr/session.h"

namespace vne::xr::samples {

/**
 * @brief Run @p sample to completion.
 * @param platform Handles the session needs on this platform (Android VM and activity); empty on desktop.
 * @param pump Optional platform event pump installed on the app.
 * @return Process exit code.
 */
[[nodiscard]] int launchSample(const SampleInfo& sample,
                               const SessionPlatformHandle& platform = {},
                               XrSampleApp::FramePump pump = {});

}  // namespace vne::xr::samples
