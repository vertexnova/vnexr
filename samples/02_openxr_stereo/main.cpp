/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

/**
 * @file main.cpp
 * @brief Sample 02 — OpenXR stereo + block interaction (Monado / Quest / PC VR).
 */

#include "stereo_cube_scene_layer.h"
#include "app/xr_sample_app.h"
#include "framework/common/logging_guard.h"
#include "vertexnova/xr/session.h"

#if defined(__ANDROID__)
#include <android_native_app_glue/android_native_app_glue.h>
#endif

#include <memory>

#if defined(__ANDROID__)

void android_main(struct android_app* android_app) {
    vne::xr::samples::LoggingGuard guard;

    vne::xr::SessionConfig config;
    config.backend = vne::xr::BackendType::eOpenXr;
    config.application_name = "02_openxr_stereo";
    config.platform.application_vm = android_app->activity->vm;
    config.platform.application_context = android_app->activity->clazz;

    vne::xr::samples::XrSampleApp app(config);
    app.pushLayer(std::make_unique<vne::xr::samples::StereoCubeSceneLayer>());

    auto session = vne::xr::createSession(config);
    if (!session) {
        return;
    }

    app.onSessionReady();

    vne::xr::Frame frame{};
    vne::xr::LayerParams layers{};
    vne::xr::InputState input{};

    android_app->onAppCmd = [](android_app* app, int32_t cmd) {
        if (cmd == APP_CMD_DESTROY) {
            app->destroyRequested = 1;
        }
    };

    while (android_app->destroyRequested == 0) {
        int events = 0;
        android_poll_source* source = nullptr;
        while (ALooper_pollOnce(0, nullptr, &events, reinterpret_cast<void**>(&source)) >= 0) {
            if (source != nullptr) {
                source->process(android_app, source);
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
        app.update(params, layers);
        session->endFrame(frame, layers);
        if (layers.request_exit) {
            break;
        }
    }
}

#else

int main() {
    vne::xr::samples::LoggingGuard guard;

    vne::xr::SessionConfig config;
    config.backend = vne::xr::BackendType::eOpenXr;
    config.application_name = "02_openxr_stereo";

    vne::xr::samples::XrSampleApp app(config);
    app.pushLayer(std::make_unique<vne::xr::samples::StereoCubeSceneLayer>());
    return app.run();
}

#endif
