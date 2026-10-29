/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "app/xr_sample_app.h"

#include <chrono>
#include <thread>

namespace vne::xr::samples {

namespace {
// Idle wait while the session is not running yet (before READY, or paused): keeps events polled without a busy spin.
constexpr auto kIdlePollInterval = std::chrono::milliseconds(10);
}  // namespace

XrSampleApp::XrSampleApp(SessionConfig config)
    : config_(std::move(config)) {}

XrSampleApp::~XrSampleApp() {
    // Layers release GPU resources on the session's device, so detach them before the session is destroyed.
    stack_.onDetach();
    session_.reset();
}

void XrSampleApp::pushLayer(std::unique_ptr<IXrLayer> layer) {
    stack_.pushLayer(std::move(layer));
}

void XrSampleApp::pushOverlay(std::unique_ptr<IXrLayer> overlay) {
    stack_.pushOverlay(std::move(overlay));
}

void XrSampleApp::setOnReady(std::function<void()> callback) {
    on_ready_ = std::move(callback);
}

void XrSampleApp::setFramePump(FramePump pump) {
    frame_pump_ = std::move(pump);
}

int XrSampleApp::run() {
    session_ = createSession(config_);
    if (!session_) {
        return 1;
    }
    onSessionReady();

    Frame frame{};
    LayerParams layers{};
    InputState input{};
    bool is_exit_requested = false;

    while (true) {
        // Ask once; the loop then runs on until the backend reports the session has ended (pollEvents() == false),
        // or stops at once when the backend cannot exit gracefully.
        const bool wants_quit = (frame_pump_ && !frame_pump_(*session_)) || layers.request_exit;
        if (wants_quit && !is_exit_requested) {
            is_exit_requested = true;
            if (!session_->requestExit()) {
                break;
            }
        }
        if (!session_->pollEvents()) {
            break;
        }
        if (!session_->beginFrame(frame)) {
            // Not running yet (waiting for READY, or paused): bound the spin while events keep being polled.
            // A failure while the session is running is transient, so retry at once.
            if (!session_->isSessionRunning()) {
                std::this_thread::sleep_for(kIdlePollInterval);
            }
            continue;
        }
        session_->pollInput(input);
        FrameParams params{frame};
        params.input = input;
        update(params, layers);
        session_->endFrame(frame, layers);
    }
    return 0;
}

void XrSampleApp::onSessionReady() {
    stack_.onAttach(session_.get());
    if (on_ready_) {
        on_ready_();
    }
}

void XrSampleApp::update(const FrameParams& params, LayerParams& out_layers) {
    stack_.onUpdate(params, out_layers);
}

}  // namespace vne::xr::samples
