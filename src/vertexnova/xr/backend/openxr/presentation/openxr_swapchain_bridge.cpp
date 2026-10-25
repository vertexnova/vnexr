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

#include "vertexnova/xr/backend/openxr/presentation/openxr_swapchain_bridge.h"

#include "vertexnova/logging/logging.h"
#include "vertexnova/xr/backend/openxr/openxr_platform_prelude.h"

#include <algorithm>
#include <optional>
#include <utility>

#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
#include "vertexnova/rhi/texture.h"
#include "vertexnova/rhi/vulkan_device_interop.h"
#endif

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.xr.openxr.swapchain");
}

namespace vne::xr {

namespace {

bool createSwapchain(XrSession session,
                     std::int64_t format,
                     std::uint32_t width,
                     std::uint32_t height,
                     std::uint32_t sample_count,
                     XrSwapchainUsageFlags usage,
                     XrSwapchain& out_swapchain,
                     std::vector<std::uint64_t>& out_vk_images) {
    XrSwapchainCreateInfo create_info{XR_TYPE_SWAPCHAIN_CREATE_INFO};
    create_info.usageFlags = usage;
    create_info.format = format;
    create_info.sampleCount = sample_count;
    create_info.width = width;
    create_info.height = height;
    create_info.faceCount = 1;
    create_info.arraySize = 1;
    create_info.mipCount = 1;

    if (XR_FAILED(xrCreateSwapchain(session, &create_info, &out_swapchain))) {
        return false;
    }

    std::uint32_t image_count = 0;
    if (XR_FAILED(xrEnumerateSwapchainImages(out_swapchain, 0, &image_count, nullptr))) {
        return false;
    }

#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
    std::vector<XrSwapchainImageVulkanKHR> images(image_count, {XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR});
    if (XR_FAILED(xrEnumerateSwapchainImages(out_swapchain,
                                             image_count,
                                             &image_count,
                                             reinterpret_cast<XrSwapchainImageBaseHeader*>(images.data())))) {
        return false;
    }
    out_vk_images.clear();
    out_vk_images.reserve(image_count);
    for (const auto& image : images) {
        out_vk_images.push_back(reinterpret_cast<std::uint64_t>(image.image));
    }
#else
    (void)out_vk_images;
#endif
    return true;
}

#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
[[nodiscard]] std::optional<vne::rhi::TextureFormat> textureFormatFromNative(std::int64_t native_format,
                                                                             bool is_depth) {
    (void)is_depth;
    switch (static_cast<VkFormat>(native_format)) {
        case VK_FORMAT_R8G8B8A8_UNORM:
            return vne::rhi::TextureFormat::eRgba8;
        case VK_FORMAT_R8G8B8A8_SRGB:
            return vne::rhi::TextureFormat::eSrgb8Alpha8;
        case VK_FORMAT_B8G8R8A8_SRGB:
            return vne::rhi::TextureFormat::eBgra8Srgb;
        case VK_FORMAT_B8G8R8A8_UNORM:
            return vne::rhi::TextureFormat::eBgra8;
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return vne::rhi::TextureFormat::eRgba16f;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return vne::rhi::TextureFormat::eRgba32f;
        case VK_FORMAT_D16_UNORM:
            return vne::rhi::TextureFormat::eDepth16;
        case VK_FORMAT_D32_SFLOAT:
            return vne::rhi::TextureFormat::eDepth32f;
        case VK_FORMAT_D24_UNORM_S8_UINT:
            return vne::rhi::TextureFormat::eDepth24Stencil8;
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return vne::rhi::TextureFormat::eDepth32fStencil8;
        default:
            return std::nullopt;
    }
}

[[nodiscard]] bool wrapSwapchainImages(vne::rhi::IDevice& device,
                                       std::uint32_t width,
                                       std::uint32_t height,
                                       std::int64_t native_format,
                                       bool is_depth,
                                       const std::vector<std::uint64_t>& vk_images,
                                       std::vector<std::shared_ptr<vne::rhi::ITexture>>& out_textures) {
    out_textures.clear();
    out_textures.reserve(vk_images.size());

    const auto mapped_format = textureFormatFromNative(native_format, is_depth);
    if (!mapped_format) {
        VNE_LOG_ERROR << "OpenXR: unsupported swapchain format " << native_format;
        return false;
    }

    vne::rhi::TextureDescriptor desc{};
    desc.name = is_depth ? "openxr_depth" : "openxr_color";
    desc.type = vne::rhi::TextureType::eTexture2d;
    desc.format = *mapped_format;
    desc.width = width;
    desc.height = height;
    desc.mip_levels = 1;
    desc.array_layers = 1;
    desc.usage = is_depth ? (vne::rhi::TextureUsage::eDepthStencil | vne::rhi::TextureUsage::eSampled)
                          : (vne::rhi::TextureUsage::eRenderTarget | vne::rhi::TextureUsage::eSampled);

    vne::rhi::vulkan::ImportedImageDescriptor import_desc;
    import_desc.texture = desc;
    import_desc.layout = is_depth ? vne::rhi::vulkan::ImportedImageLayout::eDepthStencilAttachment
                                  : vne::rhi::vulkan::ImportedImageLayout::eColorAttachment;

    for (std::size_t i = 0; i < vk_images.size(); ++i) {
        import_desc.image_handle = vk_images[i];
        auto texture = vne::rhi::vulkan::importImage(device, import_desc);
        if (!texture) {
            VNE_LOG_ERROR << "Failed to wrap OpenXR swapchain image " << i;
            out_textures.clear();
            return false;
        }
        out_textures.push_back(std::move(texture));
    }
    return true;
}
#endif

}  // namespace

OpenXrSwapchainBridge::~OpenXrSwapchainBridge() {
    destroy();
}

bool OpenXrSwapchainBridge::create(XrSession session,
                                   const std::vector<XrViewConfigurationView>& view_configs,
                                   std::int64_t color_format,
                                   std::int64_t depth_format,
                                   const std::shared_ptr<vne::rhi::IDevice>& device) {
    destroy();
    if (session == XR_NULL_HANDLE || view_configs.empty() || !device) {
        return false;
    }
    session_ = session;
    device_ = device;
    color_format_ = color_format;
    depth_format_ = depth_format;
    views_.resize(view_configs.size());

#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
    if (!vne::rhi::vulkan::getNativeDeviceHandles(*device_).has_value()) {
        VNE_LOG_ERROR << "OpenXR swapchain bridge requires a Vulkan device";
        destroy();
        return false;
    }
#endif

    for (std::size_t i = 0; i < view_configs.size(); ++i) {
        auto& view = views_[i];
        const auto& config = view_configs[i];
        view.width = config.recommendedImageRectWidth;
        view.height = config.recommendedImageRectHeight;
        const std::uint32_t sample_count = config.recommendedSwapchainSampleCount;

        if (!createSwapchain(session_,
                             color_format,
                             view.width,
                             view.height,
                             sample_count,
                             XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT,
                             view.color_swapchain,
                             view.color_vk_images)) {
            destroy();
            return false;
        }

#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
        if (!wrapSwapchainImages(*device_,
                                 view.width,
                                 view.height,
                                 color_format,
                                 false,
                                 view.color_vk_images,
                                 view.color_textures)) {
            destroy();
            return false;
        }
#endif

        if (depth_format != 0) {
            if (!createSwapchain(session_,
                                 depth_format,
                                 view.width,
                                 view.height,
                                 sample_count,
                                 XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT,
                                 view.depth_swapchain,
                                 view.depth_vk_images)) {
                destroy();
                return false;
            }
#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
            if (!wrapSwapchainImages(*device_,
                                     view.width,
                                     view.height,
                                     depth_format,
                                     true,
                                     view.depth_vk_images,
                                     view.depth_textures)) {
                destroy();
                return false;
            }
#endif
        }
    }
    return true;
}

void OpenXrSwapchainBridge::destroy() {
    for (auto& view : views_) {
        if (view.color_swapchain != XR_NULL_HANDLE) {
            xrDestroySwapchain(view.color_swapchain);
            view.color_swapchain = XR_NULL_HANDLE;
        }
        if (view.depth_swapchain != XR_NULL_HANDLE) {
            xrDestroySwapchain(view.depth_swapchain);
            view.depth_swapchain = XR_NULL_HANDLE;
        }
        view.color_vk_images.clear();
        view.depth_vk_images.clear();
        view.color_textures.clear();
        view.depth_textures.clear();
    }
    views_.clear();
    session_ = XR_NULL_HANDLE;
    device_.reset();
    cached_view_count_ = 0;
}

void OpenXrSwapchainBridge::setCachedViews(const XrView* views, std::uint32_t count) {
    cached_view_count_ = std::min(count, static_cast<std::uint32_t>(cached_views_.size()));
    for (std::uint32_t i = 0; i < cached_view_count_; ++i) {
        cached_views_[i] = views[i];
    }
}

bool OpenXrSwapchainBridge::acquireForFrame(Frame& in_out_frame) {
    if (session_ == XR_NULL_HANDLE || views_.empty()) {
        return false;
    }
    const std::uint32_t count = std::min(static_cast<std::uint32_t>(views_.size()), in_out_frame.view_count);
    in_out_frame.surfaces.view_count = count;
    for (std::uint32_t i = 0; i < count; ++i) {
        auto& view = views_[i];
        std::uint32_t index = 0;
        XrSwapchainImageAcquireInfo acquire_info{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        if (XR_FAILED(xrAcquireSwapchainImage(view.color_swapchain, &acquire_info, &index))) {
            return false;
        }
        view.acquired_index = index;

        XrSwapchainImageWaitInfo wait_info{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
        wait_info.timeout = XR_INFINITE_DURATION;
        if (XR_FAILED(xrWaitSwapchainImage(view.color_swapchain, &wait_info))) {
            return false;
        }

        if (view.depth_swapchain != XR_NULL_HANDLE) {
            std::uint32_t depth_index = 0;
            if (XR_FAILED(xrAcquireSwapchainImage(view.depth_swapchain, &acquire_info, &depth_index))) {
                return false;
            }
            if (XR_FAILED(xrWaitSwapchainImage(view.depth_swapchain, &wait_info))) {
                return false;
            }
            view.acquired_depth_index = depth_index;
            if (depth_index < view.depth_textures.size()) {
                in_out_frame.surfaces.views[i].depth_texture = view.depth_textures[depth_index];
            }
        }

        in_out_frame.surfaces.views[i].view_index = i;
        in_out_frame.surfaces.views[i].width = view.width;
        in_out_frame.surfaces.views[i].height = view.height;
        if (index < view.color_textures.size()) {
            in_out_frame.surfaces.views[i].color_texture = view.color_textures[index];
        }
    }
    return true;
}

void OpenXrSwapchainBridge::releaseForFrame() {
    for (auto& view : views_) {
        if (view.depth_swapchain != XR_NULL_HANDLE) {
            XrSwapchainImageReleaseInfo release_info{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
            xrReleaseSwapchainImage(view.depth_swapchain, &release_info);
        }
        if (view.color_swapchain != XR_NULL_HANDLE) {
            XrSwapchainImageReleaseInfo release_info{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
            xrReleaseSwapchainImage(view.color_swapchain, &release_info);
        }
    }
}

XrSwapchain OpenXrSwapchainBridge::getColorSwapchain(std::uint32_t view_index) const {
    if (view_index >= views_.size()) {
        return XR_NULL_HANDLE;
    }
    return views_[view_index].color_swapchain;
}

std::uint32_t OpenXrSwapchainBridge::getWidth(std::uint32_t view_index) const {
    if (view_index >= views_.size()) {
        return 0;
    }
    return views_[view_index].width;
}

std::uint32_t OpenXrSwapchainBridge::getHeight(std::uint32_t view_index) const {
    if (view_index >= views_.size()) {
        return 0;
    }
    return views_[view_index].height;
}

}  // namespace vne::xr
