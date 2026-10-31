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
#include "vertexnova/xr/backend/openxr/openxr_util.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <utility>

#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
#include "vertexnova/rhi/device.h"
#include "vertexnova/rhi/vulkan_device_interop.h"
#endif

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.xr.openxr.swapchain");
}

namespace vne::xr {

namespace {

/** Depth buffer owned by the bridge; the compositor gets no depth layer. */
constexpr vne::rhi::TextureFormat kDepthFormat = vne::rhi::TextureFormat::eDepth32f;

/**
 * Compositor swapchains are always single-sampled: the app resolves its own MSAA buffer into them.
 * Following `recommendedSwapchainSampleCount` would hand a multisampled image to a 1x render pass.
 */
bool createSwapchain(XrSession session,
                     std::int64_t format,
                     std::uint32_t width,
                     std::uint32_t height,
                     XrSwapchain& out_swapchain,
                     std::vector<std::uint64_t>& out_vk_images) {
    XrSwapchainCreateInfo create_info = OpenXrUtil::makeXrStruct<XrSwapchainCreateInfo>(XR_TYPE_SWAPCHAIN_CREATE_INFO);
    create_info.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
    create_info.format = format;
    create_info.sampleCount = 1;
    create_info.width = width;
    create_info.height = height;
    create_info.faceCount = 1;
    create_info.arraySize = 1;
    create_info.mipCount = 1;

    if (XR_FAILED(xrCreateSwapchain(session, &create_info, &out_swapchain))) {
        VNE_LOG_ERROR << "xrCreateSwapchain failed (" << width << "x" << height << ")";
        return false;
    }

    std::uint32_t image_count = 0;
    if (XR_FAILED(xrEnumerateSwapchainImages(out_swapchain, 0, &image_count, nullptr))) {
        return false;
    }

#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
    std::vector<XrSwapchainImageVulkanKHR> images(
        image_count,
        OpenXrUtil::makeXrStruct<XrSwapchainImageVulkanKHR>(XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR));
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
[[nodiscard]] std::optional<vne::rhi::TextureFormat> textureFormatFromNative(std::int64_t native_format) {
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
        default:
            return std::nullopt;
    }
}
#endif

/** A non-owning shared_ptr: the texture's lifetime stays tied to its ResourceRef in the device pool. */
std::shared_ptr<vne::rhi::ITexture> borrow(const vne::rhi::ResourceRef<vne::rhi::ITexture>& texture) {
    return std::shared_ptr<vne::rhi::ITexture>(texture.get(), [](vne::rhi::ITexture*) {});
}

template<typename T>
void destroyRef(vne::rhi::IDevice& device, vne::rhi::ResourceRef<T>& ref) {
    if (ref.handle().isValid()) {
        device.destroyResource(ref.handle());
    }
    ref.reset();
}

}  // namespace

OpenXrSwapchainBridge::~OpenXrSwapchainBridge() {
    destroy();
}

bool OpenXrSwapchainBridge::create(XrSession session,
                                   const std::vector<XrViewConfigurationView>& view_configs,
                                   std::int64_t color_format,
                                   std::uint32_t requested_samples,
                                   const std::shared_ptr<vne::rhi::IDevice>& device) {
    destroy();
    if (session == XR_NULL_HANDLE || view_configs.empty() || !device) {
        return false;
    }
    device_ = device;
    needs_recreate_ = false;
    ++generation_;

#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
    if (device_->getBackend() != vne::rhi::GraphicsBackend::eVulkan) {
        VNE_LOG_ERROR << "OpenXR swapchain bridge requires a Vulkan device";
        destroy();
        return false;
    }
    const auto mapped = textureFormatFromNative(color_format);
    if (!mapped) {
        VNE_LOG_ERROR << "OpenXR: unsupported swapchain format " << color_format;
        destroy();
        return false;
    }
    color_texture_format_ = *mapped;
#endif
    depth_texture_format_ = kDepthFormat;

    const auto samples = requested_samples >= 4   ? vne::rhi::SampleCount::e4
                         : requested_samples >= 2 ? vne::rhi::SampleCount::e2
                                                  : vne::rhi::SampleCount::e1;
    if (createWithSamples(session, view_configs, color_format, samples)) {
        return true;
    }
    if (samples != vne::rhi::SampleCount::e1) {
        VNE_LOG_WARN << "OpenXR: " << static_cast<std::uint32_t>(samples)
                     << "x MSAA eye buffers failed; retrying without MSAA";
        const std::uint32_t generation = generation_;
        destroy();
        device_ = device;
        generation_ = generation + 1;
        if (createWithSamples(session, view_configs, color_format, vne::rhi::SampleCount::e1)) {
            return true;
        }
    }
    destroy();
    return false;
}

bool OpenXrSwapchainBridge::createWithSamples(XrSession session,
                                              const std::vector<XrViewConfigurationView>& view_configs,
                                              std::int64_t color_format,
                                              vne::rhi::SampleCount samples) {
    session_ = session;
    sample_count_ = samples;
    views_.assign(view_configs.size(), ViewSwapchain{});

    for (std::size_t i = 0; i < view_configs.size(); ++i) {
        auto& view = views_[i];
        view.width = view_configs[i].recommendedImageRectWidth;
        view.height = view_configs[i].recommendedImageRectHeight;

        std::vector<std::uint64_t> vk_images;
        if (!createSwapchain(session_, color_format, view.width, view.height, view.color_swapchain, vk_images)) {
            return false;
        }

#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
        vne::rhi::TextureDescriptor desc{};
        desc.name = "openxr_color";
        desc.type = vne::rhi::TextureType::eTexture2d;
        desc.format = color_texture_format_;
        desc.width = view.width;
        desc.height = view.height;
        desc.mip_levels = 1;
        desc.array_layers = 1;
        desc.usage = vne::rhi::TextureUsage::eRenderTarget | vne::rhi::TextureUsage::eSampled;

        for (std::size_t image = 0; image < vk_images.size(); ++image) {
            vne::rhi::vulkan::ImportedImageDescriptor imported{};
            imported.image_handle = vk_images[image];
            imported.texture = desc;
            imported.layout = vne::rhi::vulkan::ImportedImageLayout::eColorAttachment;
            auto texture = vne::rhi::vulkan::importImage(*device_, imported);
            if (!texture) {
                VNE_LOG_ERROR << "Failed to import OpenXR swapchain image " << image;
                return false;
            }
            view.color_textures.push_back(std::move(texture));
        }
#else
        (void)vk_images;
#endif
        if (!buildViewTargets(view, samples)) {
            return false;
        }
    }
    VNE_LOG_INFO << "OpenXR swapchains ready: " << views_.size() << " views " << views_[0].width << "x"
                 << views_[0].height << ", " << views_[0].color_textures.size() << " images each, MSAA x"
                 << static_cast<std::uint32_t>(samples) << ", generation " << generation_;
    return true;
}

bool OpenXrSwapchainBridge::buildViewTargets(ViewSwapchain& view, vne::rhi::SampleCount samples) {
    using namespace vne::rhi;
    const bool is_msaa = samples != SampleCount::e1;

    TextureDescriptor depth_desc{};
    depth_desc.name = "openxr_depth";
    depth_desc.type = TextureType::eTexture2d;
    depth_desc.format = depth_texture_format_;
    depth_desc.width = view.width;
    depth_desc.height = view.height;
    depth_desc.mip_levels = 1;
    depth_desc.sample_count = samples;
    depth_desc.usage = TextureUsage::eDepthStencil;
    view.depth_texture = device_->createTexture(depth_desc);
    if (!view.depth_texture) {
        VNE_LOG_ERROR << "Failed to create the eye depth texture";
        return false;
    }
    TextureViewDescriptor depth_view_desc{};
    depth_view_desc.name = "openxr_depth_view";
    depth_view_desc.texture = borrow(view.depth_texture);
    depth_view_desc.bind_flags = TextureViewBindFlags::eViewBindDepthAttachment;
    view.depth_view = device_->createTextureView(depth_view_desc);
    if (!view.depth_view) {
        VNE_LOG_ERROR << "Failed to create the eye depth view";
        return false;
    }

    if (is_msaa) {
        TextureDescriptor msaa_desc{};
        msaa_desc.name = "openxr_color_msaa";
        msaa_desc.type = TextureType::eTexture2d;
        msaa_desc.format = color_texture_format_;
        msaa_desc.width = view.width;
        msaa_desc.height = view.height;
        msaa_desc.mip_levels = 1;
        msaa_desc.sample_count = samples;
        msaa_desc.usage = TextureUsage::eRenderTarget;
        view.msaa_texture = device_->createTexture(msaa_desc);
        if (!view.msaa_texture) {
            VNE_LOG_ERROR << "Failed to create the eye MSAA color texture";
            return false;
        }
        TextureViewDescriptor msaa_view_desc{};
        msaa_view_desc.name = "openxr_color_msaa_view";
        msaa_view_desc.texture = borrow(view.msaa_texture);
        msaa_view_desc.bind_flags = TextureViewBindFlags::eViewBindColorAttachment;
        view.msaa_view = device_->createTextureView(msaa_view_desc);
        if (!view.msaa_view) {
            VNE_LOG_ERROR << "Failed to create the eye MSAA color view";
            return false;
        }
    }

    for (std::size_t image = 0; image < view.color_textures.size(); ++image) {
        TextureViewDescriptor color_view_desc{};
        color_view_desc.name = "openxr_color_view";
        color_view_desc.texture = view.color_textures[image];
        color_view_desc.bind_flags = TextureViewBindFlags::eViewBindColorAttachment;
        auto color_view = device_->createTextureView(color_view_desc);
        if (!color_view) {
            VNE_LOG_ERROR << "Failed to create a swapchain image view";
            return false;
        }
        view.color_views.push_back(color_view);

        RenderTargetDescriptor target_desc{};
        target_desc.name = "openxr_eye_target";
        target_desc.width = view.width;
        target_desc.height = view.height;
        target_desc.formats.color_formats[0] = color_texture_format_;
        target_desc.formats.depth_stencil_format = depth_texture_format_;
        target_desc.formats.sample_count = samples;
        if (is_msaa) {
            target_desc.color_attachments[0].texture_view = view.msaa_view;
            target_desc.color_attachments[0].resolve_texture_view = color_view;
        } else {
            target_desc.color_attachments[0].texture_view = color_view;
        }
        target_desc.depth_stencil.texture_view = view.depth_view;
        auto target = device_->createRenderTarget(target_desc);
        if (!target) {
            VNE_LOG_ERROR << "Failed to create an eye render target";
            return false;
        }
        view.render_targets.push_back(target);
    }
    return true;
}

void OpenXrSwapchainBridge::destroy() {
    if (device_) {
        // Nothing may still read the images the compositor is about to take back.
        device_->waitIdle();
    }
    for (auto& view : views_) {
        if (device_) {
            for (auto& target : view.render_targets) {
                destroyRef(*device_, target);
            }
            for (auto& color_view : view.color_views) {
                destroyRef(*device_, color_view);
            }
            destroyRef(*device_, view.msaa_view);
            destroyRef(*device_, view.msaa_texture);
            destroyRef(*device_, view.depth_view);
            destroyRef(*device_, view.depth_texture);
        }
        view.render_targets.clear();
        view.color_views.clear();
        view.color_textures.clear();
        if (view.color_swapchain != XR_NULL_HANDLE) {
            xrDestroySwapchain(view.color_swapchain);
            view.color_swapchain = XR_NULL_HANDLE;
        }
    }
    views_.clear();
    session_ = XR_NULL_HANDLE;
    device_.reset();
    cached_view_count_ = 0;
    sample_count_ = vne::rhi::SampleCount::e1;
}

void OpenXrSwapchainBridge::setCachedViews(const XrView* views, std::uint32_t count) {
    cached_view_count_ = std::min(count, static_cast<std::uint32_t>(cached_views_.size()));
    for (std::uint32_t i = 0; i < cached_view_count_; ++i) {
        cached_views_[i] = views[i];
    }
}

bool OpenXrSwapchainBridge::acquireForFrame(Frame& in_out_frame) {
    if (session_ == XR_NULL_HANDLE || views_.empty() || needs_recreate_) {
        return false;
    }
    const bool has_outstanding_image =
        std::any_of(views_.begin(), views_.end(), [](const ViewSwapchain& view) { return view.is_color_acquired; });
    if (has_outstanding_image) {
        VNE_LOG_WARN << "OpenXR swapchain images from an earlier frame are still acquired; recreating swapchains";
        needs_recreate_ = true;
        return false;
    }
    const std::uint32_t count = std::min(static_cast<std::uint32_t>(views_.size()), in_out_frame.view_count);
    in_out_frame.surfaces.view_count = count;
    for (std::uint32_t i = 0; i < count; ++i) {
        auto& view = views_[i];
        std::uint32_t index = 0;
        XrSwapchainImageAcquireInfo acquire_info =
            OpenXrUtil::makeXrStruct<XrSwapchainImageAcquireInfo>(XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO);
        if (XR_FAILED(xrAcquireSwapchainImage(view.color_swapchain, &acquire_info, &index))) {
            VNE_LOG_ERROR << "xrAcquireSwapchainImage failed for view " << i;
            needs_recreate_ = true;
            return false;
        }
        view.is_color_acquired = true;
        view.is_color_waited = false;
        view.acquired_index = index;

        XrSwapchainImageWaitInfo wait_info =
            OpenXrUtil::makeXrStruct<XrSwapchainImageWaitInfo>(XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO);
        wait_info.timeout = XR_INFINITE_DURATION;
        if (XR_FAILED(xrWaitSwapchainImage(view.color_swapchain, &wait_info))) {
            // An acquired but not waited image cannot be released: only recreating the swapchain recovers.
            VNE_LOG_ERROR << "xrWaitSwapchainImage failed for view " << i;
            needs_recreate_ = true;
            return false;
        }
        view.is_color_waited = true;

        if (index >= view.render_targets.size() || index >= view.color_textures.size()) {
            VNE_LOG_ERROR << "Swapchain image index " << index << " out of range";
            return false;
        }
        auto& surface = in_out_frame.surfaces.views[i];
        surface.view_index = i;
        surface.width = view.width;
        surface.height = view.height;
        surface.render_target = view.render_targets[index].get();
        surface.color_format = color_texture_format_;
        surface.depth_format = depth_texture_format_;
        surface.sample_count = sample_count_;
        surface.swapchain_generation = generation_;
        surface.color_texture = view.color_textures[index];
    }
    return true;
}

bool OpenXrSwapchainBridge::releaseForFrame() {
    bool is_all_released = true;
    for (auto& view : views_) {
        if (!view.is_color_acquired || view.color_swapchain == XR_NULL_HANDLE) {
            continue;
        }
        if (!view.is_color_waited) {
            // Cannot be released (call order); the swapchain is rebuilt instead.
            view.is_color_acquired = false;
            needs_recreate_ = true;
            is_all_released = false;
            continue;
        }
        XrSwapchainImageReleaseInfo release_info =
            OpenXrUtil::makeXrStruct<XrSwapchainImageReleaseInfo>(XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO);
        if (XR_SUCCEEDED(xrReleaseSwapchainImage(view.color_swapchain, &release_info))) {
            view.is_color_acquired = false;
            view.is_color_waited = false;
        } else {
            VNE_LOG_WARN << "xrReleaseSwapchainImage failed";
            needs_recreate_ = true;
            is_all_released = false;
        }
    }
    return is_all_released;
}

std::uint32_t OpenXrSwapchainBridge::imageCount(std::uint32_t view_index) const {
    if (view_index >= views_.size()) {
        return 0;
    }
    return static_cast<std::uint32_t>(views_[view_index].color_textures.size());
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
