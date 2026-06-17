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

#include "vertexnova/xr/frame.h"
#include "vertexnova/xr/types.h"

#include <openxr/openxr.h>

#include <array>
#include <cstdint>
#include <vector>

struct XrSwapchain_T;
struct XrSession_T;

namespace vne::xr {

/** @brief Per-view OpenXR color + depth swapchains bridged to vnerhi textures. */
class OpenXrSwapchainBridge {
   public:
    OpenXrSwapchainBridge() = default;
    ~OpenXrSwapchainBridge();

    OpenXrSwapchainBridge(const OpenXrSwapchainBridge&) = delete;
    OpenXrSwapchainBridge& operator=(const OpenXrSwapchainBridge&) = delete;

    bool create(XrSession session,
                const std::vector<XrViewConfigurationView>& view_configs,
                std::int64_t color_format,
                std::int64_t depth_format);
    void destroy();

    bool acquireForFrame(Frame& in_out_frame);
    void releaseForFrame();

    [[nodiscard]] XrSwapchain getColorSwapchain(std::uint32_t view_index) const;
    [[nodiscard]] std::uint32_t getWidth(std::uint32_t view_index) const;
    [[nodiscard]] std::uint32_t getHeight(std::uint32_t view_index) const;
    [[nodiscard]] const std::array<XrView, 2>& cachedViews() const { return cached_views_; }
    [[nodiscard]] std::uint32_t cachedViewCount() const { return cached_view_count_; }
    void setCachedViews(const XrView* views, std::uint32_t count);

   private:
    struct ViewSwapchain {
        XrSwapchain color_swapchain = XR_NULL_HANDLE;
        XrSwapchain depth_swapchain = XR_NULL_HANDLE;
        std::vector<std::uint64_t> color_vk_images;
        std::vector<std::uint64_t> depth_vk_images;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::uint32_t acquired_index = 0;
    };

    XrSession session_ = XR_NULL_HANDLE;
    std::vector<ViewSwapchain> views_;
    std::array<XrView, 2> cached_views_{};
    std::uint32_t cached_view_count_ = 0;
};

}  // namespace vne::xr
