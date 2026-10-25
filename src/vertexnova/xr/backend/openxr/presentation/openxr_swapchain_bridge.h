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
#include <memory>
#include <vector>

namespace vne::rhi {
class IDevice;
class ITexture;
}  // namespace vne::rhi

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
                std::int64_t depth_format,
                const std::shared_ptr<vne::rhi::IDevice>& device);
    void destroy();

    /**
     * @brief Acquire and wait for the current image of every view's swapchains.
     * @return False when any acquire or wait fails; the images acquired so far stay acquired until
     * `releaseForFrame()`, which releases exactly those.
     */
    bool acquireForFrame(Frame& in_out_frame);
    /** @brief Release every image that `acquireForFrame()` acquired and has not been released yet. */
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
        std::vector<std::shared_ptr<vne::rhi::ITexture>> color_textures;
        std::vector<std::shared_ptr<vne::rhi::ITexture>> depth_textures;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::uint32_t acquired_index = 0;
        std::uint32_t acquired_depth_index = 0;
        /** Set between a successful acquire and its release; releasing an image that was not acquired is an error. */
        bool is_color_acquired = false;
        bool is_depth_acquired = false;
    };

    XrSession session_ = XR_NULL_HANDLE;
    std::shared_ptr<vne::rhi::IDevice> device_;
    std::int64_t color_format_ = 0;
    std::int64_t depth_format_ = 0;
    std::vector<ViewSwapchain> views_;
    std::array<XrView, 2> cached_views_{};
    std::uint32_t cached_view_count_ = 0;
};

}  // namespace vne::xr
