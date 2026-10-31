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
 * @file input.h
 * @brief Normalized XR controller and hand input (optional).
 */

#include "vertexnova/xr/types.h"

#include <cstdint>

namespace vne::xr {

/**
 * @brief Snapshot of controller/hand input for one frame.
 *
 * @details Index 0 is the left hand and 1 the right. Poses are in the app reference space. Buttons are levels;
 * use `InputEdges` (samples framework) for pressed/released transitions. While the app is not focused (the system
 * menu is open) everything is zero/false and `is_focused` is false.
 */
struct InputState {
    std::uint32_t controller_count = 0;
    static constexpr std::uint32_t kMaxControllers = 2;
    /** @brief Grip pose (where the hand holds the controller). */
    Pose controller_poses[kMaxControllers]{};
    bool pose_valid[kMaxControllers]{};
    /** @brief Aim pose (the pointing ray: forward is -Z). */
    Pose aim_poses[kMaxControllers]{};
    bool aim_valid[kMaxControllers]{};

    bool select_pressed[kMaxControllers]{};        //!< Trigger / pinch.
    bool squeeze_pressed[kMaxControllers]{};       //!< Grip / grasp.
    float select_value[kMaxControllers]{};         //!< Trigger value, 0..1 (pinch strength for bare hands).
    float squeeze_value[kMaxControllers]{};        //!< Grip value, 0..1.
    bool change_color_pressed[kMaxControllers]{};  //!< Primary button: X on the left, A on the right.
    bool secondary_pressed[kMaxControllers]{};     //!< Secondary button: Y on the left, B on the right.
    float thumbstick[kMaxControllers][2]{};        //!< x, y in -1..1.
    bool menu_pressed = false;                     //!< Left controller menu (hamburger) button.

    /** @brief False while the system UI has input focus; the app should hide pointers and ignore input. */
    bool is_focused = true;
};

}  // namespace vne::xr
