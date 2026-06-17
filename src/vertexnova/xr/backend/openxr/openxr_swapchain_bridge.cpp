/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "vertexnova/xr/backend/openxr/openxr_swapchain_bridge.h"

#include <openxr/openxr.h>

#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
#include <openxr/openxr_platform.h>
#include <vulkan/vulkan.h>
#endif

#include "vertexnova/xr/backend/openxr/openxr_vulkan_texture.h"

namespace vne::xr {

OpenXrSwapchainBridge::~OpenXrSwapchainBridge() {
    destroy();
}

bool OpenXrSwapchainBridge::create(void* session, std::uint32_t view_count, std::uint32_t width, std::uint32_t height) {
    destroy();
    if (session == nullptr || view_count == 0) {
        return false;
    }
    session_ = static_cast<XrSession_T*>(session);
    view_count_ = view_count;
    views_.resize(view_count);

    for (std::uint32_t i = 0; i < view_count; ++i) {
        auto& view = views_[i];
        view.width = width;
        view.height = height;

        XrSwapchainCreateInfo create_info{XR_TYPE_SWAPCHAIN_CREATE_INFO};
        create_info.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
        create_info.format = static_cast<int64_t>(VK_FORMAT_R8G8B8A8_SRGB);
#else
        create_info.format = 43;  // DXGI/D3D fallback placeholder
#endif
        create_info.sampleCount = 1;
        create_info.width = width;
        create_info.height = height;
        create_info.faceCount = 1;
        create_info.arraySize = 1;
        create_info.mipCount = 1;

        XrSwapchain swapchain = XR_NULL_HANDLE;
        if (XR_FAILED(xrCreateSwapchain(session_, &create_info, &swapchain))) {
            destroy();
            return false;
        }
        view.swapchain = swapchain;

        std::uint32_t image_count = 0;
        if (XR_FAILED(xrEnumerateSwapchainImages(swapchain, 0, &image_count, nullptr))) {
            destroy();
            return false;
        }
        std::vector<XrSwapchainImageVulkanKHR> images(image_count, {XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR});
        if (XR_FAILED(xrEnumerateSwapchainImages(swapchain,
                                                 image_count,
                                                 &image_count,
                                                 reinterpret_cast<XrSwapchainImageBaseHeader*>(images.data())))) {
            destroy();
            return false;
        }
        view.vk_images.reserve(image_count);
        for (const auto& image : images) {
            view.vk_images.push_back(reinterpret_cast<std::uint64_t>(image.image));
        }
    }
    return true;
}

void OpenXrSwapchainBridge::destroy() {
    for (auto& view : views_) {
        if (view.swapchain != XR_NULL_HANDLE && session_ != nullptr) {
            xrDestroySwapchain(view.swapchain);
            view.swapchain = XR_NULL_HANDLE;
        }
        view.vk_images.clear();
    }
    views_.clear();
    view_count_ = 0;
    session_ = nullptr;
}

bool OpenXrSwapchainBridge::acquireForFrame(Frame& in_out_frame) {
    if (session_ == nullptr || views_.empty()) {
        return false;
    }
    const std::uint32_t count = std::min(view_count_, in_out_frame.view_count);
    in_out_frame.surfaces.view_count = count;
    for (std::uint32_t i = 0; i < count; ++i) {
        auto& view = views_[i];
        std::uint32_t index = 0;
        XrSwapchainImageAcquireInfo acquire_info{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        if (XR_FAILED(xrAcquireSwapchainImage(view.swapchain, &acquire_info, &index))) {
            return false;
        }
        view.acquired_index = index;

        XrSwapchainImageWaitInfo wait_info{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
        wait_info.timeout = XR_INFINITE_DURATION;
        if (XR_FAILED(xrWaitSwapchainImage(view.swapchain, &wait_info))) {
            return false;
        }

        in_out_frame.surfaces.views[i].view_index = i;
        in_out_frame.surfaces.views[i].width = view.width;
        in_out_frame.surfaces.views[i].height = view.height;
        if (index < view.vk_images.size()) {
            void* vk_image = reinterpret_cast<void*>(view.vk_images[index]);
            in_out_frame.surfaces.views[i].color_texture = makeOpenXrVulkanTexture(vk_image, view.width, view.height);
        }
    }
    return true;
}

void OpenXrSwapchainBridge::releaseForFrame() {
    for (auto& view : views_) {
        if (view.swapchain == XR_NULL_HANDLE) {
            continue;
        }
        XrSwapchainImageReleaseInfo release_info{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        xrReleaseSwapchainImage(view.swapchain, &release_info);
    }
}

}  // namespace vne::xr
