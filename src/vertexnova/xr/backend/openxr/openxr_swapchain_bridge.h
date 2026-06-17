#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "vertexnova/xr/frame.h"

#include <cstdint>
#include <vector>

struct XrSwapchain_T;
struct XrSession_T;

namespace vne::xr {

/** @brief Per-view OpenXR swapchain + Vulkan image bridge. */
class OpenXrSwapchainBridge {
   public:
    OpenXrSwapchainBridge() = default;
    ~OpenXrSwapchainBridge();

    OpenXrSwapchainBridge(const OpenXrSwapchainBridge&) = delete;
    OpenXrSwapchainBridge& operator=(const OpenXrSwapchainBridge&) = delete;

    bool create(void* session, std::uint32_t view_count, std::uint32_t width, std::uint32_t height);
    void destroy();

    bool acquireForFrame(Frame& in_out_frame);
    void releaseForFrame();

   private:
    struct ViewSwapchain {
        XrSwapchain_T* swapchain = nullptr;
        std::vector<std::uint64_t> vk_images;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::uint32_t acquired_index = 0;
    };

    XrSession_T* session_ = nullptr;
    std::vector<ViewSwapchain> views_;
    std::uint32_t view_count_ = 0;
};

}  // namespace vne::xr
