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

#include <openxr/openxr.h>

namespace vne::xr {

/** @brief Optional XR_EXT_debug_utils messenger (dev builds). */
class OpenXrDebugMessenger final {
   public:
    [[nodiscard]] bool create(XrInstance instance, PFN_xrGetInstanceProcAddr get_proc);
    void destroy(XrInstance instance);

   private:
    XrDebugUtilsMessengerEXT messenger_ = XR_NULL_HANDLE;
};

}  // namespace vne::xr
