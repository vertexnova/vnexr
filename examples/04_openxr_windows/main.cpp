/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "common/logging_guard.h"
#include "win32_loop.h"
#include "vertexnova/xr/session.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    vne::xr::examples::LoggingGuard logging_guard;

    vne::xr::SessionConfig config;
    config.backend = vne::xr::BackendType::eOpenXr;
    config.application_name = "04_openxr_windows";

    auto session = vne::xr::createSession(config);
    vne::xr::examples::Win32Loop loop;
    vne::xr::examples::Win32OpenXrApp app(loop);
    session->run(app);
    return 0;
}
