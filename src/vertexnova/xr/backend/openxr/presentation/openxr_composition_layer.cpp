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

#include "vertexnova/xr/backend/openxr/presentation/openxr_composition_layer.h"

#include <algorithm>

#include "vertexnova/xr/backend/openxr/presentation/openxr_swapchain_bridge.h"

namespace vne::xr {

void OpenXrCompositionLayer::buildProjectionLayer(XrSpace space,
                                                  const OpenXrSwapchainBridge& bridge,
                                                  std::uint32_t view_count,
                                                  XrCompositionLayerProjection& out_layer,
                                                  std::array<XrCompositionLayerProjectionView, 2>& out_views) const {
    out_layer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    out_layer.layerFlags =
        XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT | XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
    out_layer.space = space;

    const std::uint32_t count = std::min(view_count, static_cast<std::uint32_t>(out_views.size()));
    const auto& cached = bridge.cachedViews();
    for (std::uint32_t i = 0; i < count; ++i) {
        out_views[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
        out_views[i].pose = cached[i].pose;
        out_views[i].fov = cached[i].fov;
        out_views[i].subImage.swapchain = bridge.getColorSwapchain(i);
        out_views[i].subImage.imageRect.offset = {0, 0};
        out_views[i].subImage.imageRect.extent = {static_cast<int32_t>(bridge.getWidth(i)),
                                                  static_cast<int32_t>(bridge.getHeight(i))};
        out_views[i].subImage.imageArrayIndex = 0;
    }
    out_layer.viewCount = count;
    out_layer.views = out_views.data();
}

}  // namespace vne::xr
