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

#include <cstdint>

namespace vne::xr {

/** @brief Platform-specific handles for OpenXR instance/session creation. */
struct OpenXrPlatformHandle {
#if defined(_WIN32)
    void* hwnd = nullptr;
    void* hinstance = nullptr;
#endif
#if defined(__ANDROID__)
    void* application_vm = nullptr;
    void* application_context = nullptr;
#endif
};

}  // namespace vne::xr
