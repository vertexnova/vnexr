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

/** @brief Snapshot of controller/hand input for one frame. */
struct InputState {
    std::uint32_t controller_count = 0;
    static constexpr std::uint32_t kMaxControllers = 2;
    Pose controller_poses[kMaxControllers]{};
    bool select_pressed[kMaxControllers]{};
    bool squeeze_pressed[kMaxControllers]{};
};

}  // namespace vne::xr
