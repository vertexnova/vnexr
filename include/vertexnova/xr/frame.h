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
#include "vertexnova/xr/input.h"

#include <cstdint>

namespace vne::xr {

/** @brief Per-frame timing and view data from the compositor. */
struct Frame {
    std::uint64_t frame_index = 0;
    double predicted_display_time = 0.0;
    bool should_render = false;
    std::uint32_t view_count = 0;
    static constexpr std::uint32_t kMaxViews = 2;
    View views[kMaxViews];
    /** @brief Head (VIEW space) pose in the app reference space; meaningful when `is_head_pose_valid`. */
    Pose head_pose{};
    bool is_head_pose_valid = false;
    /**
     * @brief Increases each time the reference space moves under the app (recenter, or the runtime settling the
     * floor origin after start). World-locked content placed in front of the user should be placed again.
     */
    std::uint32_t reference_space_epoch = 0;
    SurfaceTextures surfaces{};
};

/** @brief Host → app frame parameters: the compositor frame plus the input snapshot. */
struct FrameParams {
    Frame frame{};
    InputState input{};
};

/** @brief App → host layer hints: depth range, passthrough, exit request and layer submission. */
struct LayerParams {
    float near_z = kDefaultNearZ;
    float far_z = kDefaultFarZ;
    bool enable_passthrough = false;
    bool request_exit = false;
    /**
     * @brief True only when every eye of this frame was fully drawn. When false, endFrame submits zero layers, so
     * the compositor never shows an image nobody rendered. Starts false each frame; the renderer sets it.
     */
    bool submit_projection = false;
    /** @brief Background colour (linear RGB) the eye buffers are cleared to before layers draw. */
    float clear_color[3]{0.02f, 0.02f, 0.04f};
};

}  // namespace vne::xr
