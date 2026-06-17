/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "common/logging_guard.h"
#include "stereo_cube_renderer.h"
#include "vertexnova/xr/session.h"

int main() {
    vne::xr::examples::LoggingGuard logging_guard;

    vne::xr::SessionConfig config;
    config.backend = vne::xr::BackendType::eOpenXr;
    config.application_name = "03_hello_openxr";

    auto session = vne::xr::createSession(config);
    vne::xr::examples::StereoCubeRenderer renderer;
    session->run(renderer);
    return 0;
}
