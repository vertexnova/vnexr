#pragma once
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

#include "vertexnova/xr/types.h"
#include "vertexnova/xr/view.h"

#include <openxr/openxr.h>

namespace vne::xr {

/** @brief OpenXR-specific projection and pose math (Diligent XrCreateProjectionFov). */
namespace OpenXrMath {

void poseToMatrix(const Pose& pose, float out_matrix[16]);
void createProjectionFov(const FieldOfView& fov, float near_z, float far_z, float out_matrix[16]);
void populateViewMatrices(const XrView& xr_view, float near_z, float far_z, View& out_view);

}  // namespace OpenXrMath

}  // namespace vne::xr
