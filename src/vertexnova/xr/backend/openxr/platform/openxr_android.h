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

#include "vertexnova/xr/xr_error.h"

#include <openxr/openxr.h>

namespace vne::xr {

/** @brief Android xrInitializeLoaderKHR wrapper. */
[[nodiscard]] Result initializeOpenXrLoaderAndroid(void* application_vm, void* application_context);

}  // namespace vne::xr
