/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "win32_loop.h"

#include "vertexnova/logging/logging.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace vne::xr_ns::examples {

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

void Win32OpenXrApp::update(const FrameParams& params, LayerParams& out_layers) {
    if (!loop_.pump_messages()) {
        out_layers.request_exit = true;
        return;
    }
    if (params.frame.frame_index % 120 == 0) {
        VNE_LOG_INFO << "Windows OpenXR frame " << params.frame.frame_index;
    }
}

}  // namespace vne::xr_ns::examples
