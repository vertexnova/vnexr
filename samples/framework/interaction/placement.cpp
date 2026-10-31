/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "interaction/placement.h"

#include <algorithm>
#include <cmath>

namespace vne::xr::samples {

namespace {
/**
 * How far (in half heights) an object's bottom may be below a support's top and still land on it: a hand never
 * lines up exactly, so a block let go slightly inside the table should end up on it, not on the floor.
 */
constexpr float kSinkSlack = 1.f;
}  // namespace

xrmath::Quat uprightYaw(xrmath::Quat orientation) {
    const xrmath::Quat q = xrmath::normalized(orientation);
    xrmath::Vec3 forward = xrmath::rotate(q, {0.f, 0.f, -1.f});
    forward.y = 0.f;
    if (xrmath::length(forward) < 1e-3f) {
        // Pointing straight up or down: the side axis still carries the heading.
        const xrmath::Vec3 right = xrmath::rotate(q, {1.f, 0.f, 0.f});
        forward = {-right.z, 0.f, right.x};
    }
    forward = xrmath::normalized(forward);
    return xrmath::quatFromAxisAngle({0.f, 1.f, 0.f}, std::atan2(-forward.x, -forward.z));
}

float restingHeight(xrmath::Vec3 position,
                    float half_height,
                    float footprint_half,
                    const std::vector<SupportBox>& supports,
                    float floor_y) {
    const float bottom = position.y - half_height;
    float height = floor_y;
    for (const SupportBox& support : supports) {
        const float reach_x = support.half_extent.x + footprint_half * 0.5f;
        const float reach_z = support.half_extent.z + footprint_half * 0.5f;
        if (std::abs(position.x - support.center.x) > reach_x || std::abs(position.z - support.center.z) > reach_z) {
            continue;
        }
        if (support.top() > bottom + half_height * kSinkSlack) {
            continue;
        }
        height = std::max(height, support.top());
    }
    return height;
}

}  // namespace vne::xr::samples
