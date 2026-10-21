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
 * @file types.h
 * @brief XR backend identifiers, session state, pose, and field-of-view types.
 */

#include "vertexnova/xr/export.h"

#include <cstdint>

namespace vne::xr {

/** @brief XR compositor / session backend identifier. */
enum class BackendType : std::uint8_t {
    eNull = 0,
    eOpenXr,
    eVisionOs,
};

/** @brief High-level XR session lifecycle state. */
enum class SessionState : std::uint8_t {
    eUnknown = 0,
    eIdle,
    eReady,
    eSynchronized,
    eVisible,
    eFocused,
    eStopping,
    eLossPending,
    eExiting,
};

/** @brief 3D pose: position (meters) + orientation quaternion (x, y, z, w). */
struct Pose {
    float position[3]{0.f, 0.f, 0.f};
    float orientation[4]{0.f, 0.f, 0.f, 1.f};
};

/** @brief Horizontal and vertical field of view (radians). */
struct FieldOfView {
    float angle_left = 0.f;
    float angle_right = 0.f;
    float angle_up = 0.f;
    float angle_down = 0.f;
};

/** @brief Element count of a row-major 4×4 float matrix. */
inline constexpr std::uint32_t kMat4FloatCount = 16;

/** @brief Default near clip plane distance (meters). */
inline constexpr float kDefaultNearZ = 0.1f;

/** @brief Default far clip plane distance (meters). */
inline constexpr float kDefaultFarZ = 100.f;

}  // namespace vne::xr
