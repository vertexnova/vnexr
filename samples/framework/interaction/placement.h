#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "common/xr_math.h"

#include <vector>

namespace vne::xr::samples {

/** @brief An upright box something can rest on (a table, another block). Axis-aligned in scene space. */
struct SupportBox {
    xrmath::Vec3 center{};
    xrmath::Vec3 half_extent{};

    [[nodiscard]] float top() const { return center.y + half_extent.y; }
};

/** @brief @p orientation with its tilt removed: only the heading (rotation about +Y) is kept. */
[[nodiscard]] xrmath::Quat uprightYaw(xrmath::Quat orientation);

/**
 * @brief Height of the highest support top under a released object, or @p floor_y when nothing is below.
 *
 * @param position Center of the released object.
 * @param half_height Half of its height; supports whose top is above its bottom (plus a little slack) are ignored,
 * so an object let go under a table falls to the floor rather than jumping onto the table.
 * @param footprint_half Half of its width; the object rests on a support when its center is over the support's top
 * extended by half of this, so one hanging mostly off an edge falls past it.
 */
[[nodiscard]] float restingHeight(xrmath::Vec3 position,
                                  float half_height,
                                  float footprint_half,
                                  const std::vector<SupportBox>& supports,
                                  float floor_y = 0.f);

}  // namespace vne::xr::samples
