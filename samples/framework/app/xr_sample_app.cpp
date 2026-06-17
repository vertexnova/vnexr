/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "app/xr_sample_app.h"

namespace vne::xr::samples {

XrSampleApp::XrSampleApp(SessionConfig config)
    : config_(std::move(config)) {}

XrSampleApp::~XrSampleApp() {
    stack_.onDetach();
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

int XrSampleApp::run() {
    session_ = createSession(config_);
    if (!session_) {
        return 1;
    }
    session_->run(*this);
    return 0;
}

void XrSampleApp::onSessionReady() {
    stack_.onAttach();
    if (on_ready_) {
        on_ready_();
    }
}

void XrSampleApp::update(const FrameParams& params, LayerParams& out_layers) {
    stack_.onUpdate(params, out_layers);
}

}  // namespace vne::xr::samples
