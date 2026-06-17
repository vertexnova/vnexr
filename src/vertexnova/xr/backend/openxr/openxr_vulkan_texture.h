#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#ifdef VNE_XR_WITH_RHI

#include "vertexnova/rhi/texture.h"

#include <cstdint>
#include <memory>

namespace vne::xr_ns {

/**
 * @brief Non-owning vne::rhi::ITexture view over an OpenXR Vulkan swapchain image.
 *
 * Compositor-owned VkImage lifetime; vnexr only wraps the handle for render hooks.
 */
class OpenXrVulkanTexture final : public vne::rhi::ITexture {
   public:
    OpenXrVulkanTexture(void* vk_image, std::uint32_t width, std::uint32_t height);

    [[nodiscard]] bool initialize(const vne::rhi::TextureDescriptor& desc) override;
    [[nodiscard]] const vne::rhi::TextureDescriptor& descriptor() const override;
    [[nodiscard]] std::uint32_t getWidth() const override;
    [[nodiscard]] std::uint32_t getHeight() const override;
    [[nodiscard]] std::uint32_t getDepth() const override;
    [[nodiscard]] std::uint32_t getMipLevels() const override;
    [[nodiscard]] std::uint32_t getArrayLayers() const override;
    [[nodiscard]] bool isWrappedImage() const override;
    [[nodiscard]] std::string getName() const override;
    [[nodiscard]] vne::rhi::ResourceType getResourceType() const override;
    [[nodiscard]] bool isValid() const override;
    void destroy() override;
    [[nodiscard]] void* nativeHandle() const override;

   private:
    void* vk_image_ = nullptr;
    vne::rhi::TextureDescriptor descriptor_{};
};

[[nodiscard]] std::shared_ptr<vne::rhi::ITexture> make_openxr_vulkan_texture(void* vk_image,
                                                                             std::uint32_t width,
                                                                             std::uint32_t height);

}  // namespace vne::xr_ns

#endif  // VNE_XR_WITH_RHI
