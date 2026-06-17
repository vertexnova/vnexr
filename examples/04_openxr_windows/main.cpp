/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   June 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

#include "common/logging_guard.h"
#include "win32_loop.h"
#include "vertexnova/xr/session.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int) {
    vne::xr::examples::LoggingGuard logging_guard;

    vne::xr::SessionConfig config;
    config.backend = vne::xr::BackendType::eOpenXr;
    config.application_name = "04_openxr_windows";
    config.platform.hinstance = instance;

    auto session = vne::xr::createSession(config);
    vne::xr::examples::Win32Loop loop;
    vne::xr::examples::Win32OpenXrApp app(loop);

    app.onSessionReady();
    vne::xr::Frame frame{};
    vne::xr::LayerParams layers{};
    vne::xr::InputState input{};

    while (!loop.should_quit()) {
        if (!loop.pump_messages()) {
            break;
        }
        if (!session->pollEvents()) {
            break;
        }
        if (!session->isSessionRunning()) {
            continue;
        }
        if (!session->beginFrame(frame)) {
            continue;
        }
        session->pollInput(input);
        vne::xr::FrameParams params{frame};
        params.input = input;
        app.update(params, layers);
        session->endFrame(frame, layers);
        if (layers.request_exit) {
            break;
        }
    }
    return 0;
}
