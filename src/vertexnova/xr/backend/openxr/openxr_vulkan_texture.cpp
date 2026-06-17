/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "vertexnova/xr/backend/openxr/openxr_vulkan_texture.h"

namespace vne::xr {

OpenXrVulkanTexture::OpenXrVulkanTexture(void* vk_image, std::uint32_t width, std::uint32_t height) {
    vk_image_ = vk_image;
    descriptor_.name = "openxr_swapchain";
    descriptor_.type = vne::rhi::TextureType::eTexture2d;
    descriptor_.format = vne::rhi::TextureFormat::eRgba8;
    descriptor_.width = width;
    descriptor_.height = height;
    descriptor_.usage = vne::rhi::TextureUsage::eColorAttachment;
}

bool OpenXrVulkanTexture::initialize(const vne::rhi::TextureDescriptor& desc) {
    descriptor_ = desc;
    return vk_image_ != nullptr;
}

const vne::rhi::TextureDescriptor& OpenXrVulkanTexture::descriptor() const {
    return descriptor_;
}

std::uint32_t OpenXrVulkanTexture::getWidth() const {
    return descriptor_.width;
}

std::uint32_t OpenXrVulkanTexture::getHeight() const {
    return descriptor_.height;
}

std::uint32_t OpenXrVulkanTexture::getDepth() const {
    return descriptor_.depth;
}

std::uint32_t OpenXrVulkanTexture::getMipLevels() const {
    return 1;
}

std::uint32_t OpenXrVulkanTexture::getArrayLayers() const {
    return 1;
}

bool OpenXrVulkanTexture::isWrappedImage() const {
    return true;
}

std::string OpenXrVulkanTexture::getName() const {
    return descriptor_.name;
}

vne::rhi::ResourceType OpenXrVulkanTexture::getResourceType() const {
    return vne::rhi::ResourceType::eTexture;
}

bool OpenXrVulkanTexture::isValid() const {
    return vk_image_ != nullptr;
}

void OpenXrVulkanTexture::destroy() {
    vk_image_ = nullptr;
}

void* OpenXrVulkanTexture::nativeHandle() const {
    return vk_image_;
}

std::shared_ptr<vne::rhi::ITexture> makeOpenXrVulkanTexture(void* vk_image, std::uint32_t width, std::uint32_t height) {
    return std::make_shared<OpenXrVulkanTexture>(vk_image, width, height);
}

}  // namespace vne::xr
