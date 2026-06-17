#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   February 2026
 *
 * Autodoc:   yes
 * ---------------------------------------------------------------------- */

/**
 * @file xr.h
 * @brief VertexNova XR — library entry points.
 */

#include "vertexnova/xr/export.h"

namespace vne::xr {

/** @brief Returns the project version string. */
[[nodiscard]] VNE_XR_API const char* getVersion();

/** @brief Returns a greeting string. */
[[nodiscard]] VNE_XR_API const char* hello();

}  // namespace vne::xr
