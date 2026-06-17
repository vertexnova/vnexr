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

#if defined(__ANDROID__)

#include "common/logging_guard.h"
#include "stereo_cube_renderer.h"
#include "vertexnova/xr/session.h"

#include <android_native_app_glue/android_native_app_glue.h>

static void android_main(struct android_app* app) {
    vne::xr::examples::LoggingGuard logging_guard;

    vne::xr::SessionConfig config;
    config.backend = vne::xr::BackendType::eOpenXr;
    config.application_name = "03_hello_openxr";
    config.platform.application_vm = app->activity->vm;
    config.platform.application_context = app->activity->clazz;

    auto session = vne::xr::createSession(config);
    vne::xr::examples::StereoCubeRenderer renderer;

    app->onAppCmd = [](android_app* android_app, int32_t cmd) {
        if (cmd == APP_CMD_DESTROY) {
            android_app->destroyRequested = 1;
        }
    };

    renderer.onSessionReady();
    vne::xr::Frame frame{};
    vne::xr::LayerParams layers{};
    vne::xr::InputState input{};

    while (app->destroyRequested == 0) {
        int events = 0;
        android_poll_source* source = nullptr;
        while (ALooper_pollOnce(0, nullptr, &events, reinterpret_cast<void**>(&source)) >= 0) {
            if (source != nullptr) {
                source->process(app, source);
            }
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
        renderer.update(params, layers);
        session->endFrame(frame, layers);
        if (layers.request_exit) {
            break;
        }
    }
}

#else

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

#endif
