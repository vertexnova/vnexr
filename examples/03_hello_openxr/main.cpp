/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "common/logging_guard.h"
#include "stereo_cube_renderer.h"
#include "vertexnova/xr/session.h"

int main() {
    vne::xr_ns::examples::LoggingGuard logging_guard;

    vne::xr_ns::SessionConfig config;
    config.backend = vne::xr_ns::BackendType::eOpenXr;
    config.application_name = "03_hello_openxr";

    auto session = vne::xr_ns::create_session(config);
    vne::xr_ns::examples::StereoCubeRenderer renderer;
    session->run(renderer);
    return 0;
}
