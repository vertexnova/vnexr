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

#include <array>
#include <cstdint>

namespace vne::xr {

class OpenXrSwapchainBridge;

/** @brief Builds XrCompositionLayerProjectionView arrays for xrEndFrame. */
class OpenXrCompositionLayer final {
   public:
    void buildProjectionLayer(XrSpace space,
                              const OpenXrSwapchainBridge& bridge,
                              std::uint32_t view_count,
                              XrCompositionLayerProjection& out_layer,
                              std::array<XrCompositionLayerProjectionView, 2>& out_views) const;
};

}  // namespace vne::xr
