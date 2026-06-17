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
 * @file view.h
 * @brief Single XR view (eye) pose and projection inputs.
 */

#include "vertexnova/xr/types.h"

#include <cstdint>

namespace vne::xr {

/** @brief Single XR view (eye) pose and projection inputs. */
struct View {
    std::uint32_t view_index = 0;
    Pose pose{};
    FieldOfView fov{};
    /** @brief Row-major 4x4 view matrix (world → eye). */
    float view_matrix[16]{};
    /** @brief Row-major 4x4 projection matrix. */
    float projection_matrix[16]{};
};

}  // namespace vne::xr
