#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   June 2026
 *
 * Autodoc:   yes
 * ---------------------------------------------------------------------- */

/**
 * @file frame.h
 * @brief Per-frame timing, views, and layer parameters.
 */

#include "vertexnova/xr/swapchain_image.h"
#include "vertexnova/xr/view.h"

#include <cstdint>

namespace vne::xr {

/** @brief Per-frame timing and view data from the compositor. */
struct Frame {
    std::uint64_t frame_index = 0;
    double predicted_display_time = 0.0;
    bool should_render = false;
    std::uint32_t view_count = 0;
    static constexpr std::uint32_t kMaxViews = 2;
    View views[kMaxViews]{};
    SurfaceTextures surfaces{};
};

/** @brief Host → app frame parameters (ShellParams pattern from IGL). */
struct FrameParams {
    Frame frame{};
};

/** @brief App → host layer hints (AppParams pattern from IGL). */
struct LayerParams {
    float near_z = 0.1f;
    float far_z = 100.f;
    bool enable_passthrough = false;
    bool request_exit = false;
};

}  // namespace vne::xr
