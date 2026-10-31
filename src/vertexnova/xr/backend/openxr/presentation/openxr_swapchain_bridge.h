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

#include "vertexnova/rhi/render_target.h"
#include "vertexnova/rhi/resource_ref.h"
#include "vertexnova/rhi/texture.h"
#include "vertexnova/rhi/texture_view.h"

#include <openxr/openxr.h>

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace vne::rhi {
class IDevice;
}  // namespace vne::rhi

namespace vne::xr {

/**
 * @brief Per-view OpenXR color swapchains bridged to vnerhi render targets.
 *
 * @details For every view the bridge imports each compositor image, owns one depth (and, with MSAA, one color)
 * attachment, and builds one render target per image index when the swapchain is created. Frames then only look
 * a target up, so nothing is created or destroyed per frame.
 */
class OpenXrSwapchainBridge {
   public:
    OpenXrSwapchainBridge() = default;
    ~OpenXrSwapchainBridge();

    OpenXrSwapchainBridge(const OpenXrSwapchainBridge&) = delete;
    OpenXrSwapchainBridge& operator=(const OpenXrSwapchainBridge&) = delete;

    /**
     * @param requested_samples MSAA sample count for the eye buffers. Falls back to 1 when the device cannot
     * build the multisampled attachments.
     */
    bool create(XrSession session,
                const std::vector<XrViewConfigurationView>& view_configs,
                std::int64_t color_format,
                std::uint32_t requested_samples,
                const std::shared_ptr<vne::rhi::IDevice>& device);
    /** @brief Destroy everything. The caller must have waited for the GPU to go idle first. */
    void destroy();

    /**
     * @brief Acquire and wait for the current image of every view's swapchain.
     * @return False when any acquire or wait fails, or when images from an earlier frame are still outstanding.
     * The images acquired so far stay acquired until `releaseForFrame()`, which releases exactly those.
     */
    bool acquireForFrame(Frame& in_out_frame);
    /**
     * @brief Release every image that `acquireForFrame()` acquired and has not been released yet.
     * @return True when nothing is left outstanding.
     */
    [[nodiscard]] bool releaseForFrame();

    /** @brief True after an acquire/wait/release failure left an image stuck: the swapchains must be recreated. */
    [[nodiscard]] bool needsRecreate() const { return needs_recreate_; }
    [[nodiscard]] bool isCreated() const { return !views_.empty(); }
    [[nodiscard]] std::uint32_t generation() const { return generation_; }
    [[nodiscard]] vne::rhi::SampleCount sampleCount() const { return sample_count_; }
    [[nodiscard]] std::uint32_t imageCount(std::uint32_t view_index) const;

    [[nodiscard]] XrSwapchain getColorSwapchain(std::uint32_t view_index) const;
    [[nodiscard]] std::uint32_t getWidth(std::uint32_t view_index) const;
    [[nodiscard]] std::uint32_t getHeight(std::uint32_t view_index) const;
    [[nodiscard]] const std::array<XrView, 2>& cachedViews() const { return cached_views_; }
    [[nodiscard]] std::uint32_t cachedViewCount() const { return cached_view_count_; }
    void setCachedViews(const XrView* views, std::uint32_t count);

   private:
    struct ViewSwapchain {
        XrSwapchain color_swapchain = XR_NULL_HANDLE;
        std::vector<std::shared_ptr<vne::rhi::ITexture>> color_textures;
        std::vector<vne::rhi::ResourceRef<vne::rhi::ITextureView>> color_views;
        std::vector<vne::rhi::ResourceRef<vne::rhi::IRenderTarget>> render_targets;
        /** Shared by all image indices of this view: the GPU is idle before a new frame begins. */
        vne::rhi::ResourceRef<vne::rhi::ITexture> depth_texture;
        vne::rhi::ResourceRef<vne::rhi::ITextureView> depth_view;
        vne::rhi::ResourceRef<vne::rhi::ITexture> msaa_texture;
        vne::rhi::ResourceRef<vne::rhi::ITextureView> msaa_view;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::uint32_t acquired_index = 0;
        /** Set between a successful acquire and its release; releasing an image that was not acquired is an error. */
        bool is_color_acquired = false;
        /** Set after a successful xrWaitSwapchainImage; xrReleaseSwapchainImage requires it. */
        bool is_color_waited = false;
    };

    [[nodiscard]] bool createWithSamples(XrSession session,
                                         const std::vector<XrViewConfigurationView>& view_configs,
                                         std::int64_t color_format,
                                         vne::rhi::SampleCount samples);
    [[nodiscard]] bool buildViewTargets(ViewSwapchain& view, vne::rhi::SampleCount samples);

    XrSession session_ = XR_NULL_HANDLE;
    std::shared_ptr<vne::rhi::IDevice> device_;
    vne::rhi::TextureFormat color_texture_format_ = vne::rhi::TextureFormat::eUnknown;
    vne::rhi::TextureFormat depth_texture_format_ = vne::rhi::TextureFormat::eUnknown;
    vne::rhi::SampleCount sample_count_ = vne::rhi::SampleCount::e1;
    std::vector<ViewSwapchain> views_;
    std::array<XrView, 2> cached_views_{};
    std::uint32_t cached_view_count_ = 0;
    std::uint32_t generation_ = 0;
    bool needs_recreate_ = false;
};

}  // namespace vne::xr
