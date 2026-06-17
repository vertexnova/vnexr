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

#include "win32_loop.h"

#include "vertexnova/logging/logging.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace vne::xr::examples {

bool Win32Loop::pump_messages() {
    MSG msg{};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            quit_ = true;
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return !quit_;
}

void Win32Loop::request_quit() {
    quit_ = true;
}

bool Win32Loop::should_quit() const {
    return quit_;
}

Win32OpenXrApp::Win32OpenXrApp(Win32Loop& loop) : loop_(loop) {}

void Win32OpenXrApp::onSessionReady() {
    VNE_LOG_INFO << "Windows OpenXR session ready";
}

void Win32OpenXrApp::update(const FrameParams& params, LayerParams& out_layers) {
    if (!loop_.pump_messages()) {
        out_layers.request_exit = true;
        return;
    }
    if (params.frame.should_render) {
        ++frames_seen_;
        if (frames_seen_ % 120 == 0) {
            VNE_LOG_INFO << "Windows OpenXR frame " << params.frame.frame_index << " views="
                         << params.frame.view_count;
        }
    }
    if (frames_seen_ >= 3600) {
        out_layers.request_exit = true;
    }
}

}  // namespace vne::xr::examples
