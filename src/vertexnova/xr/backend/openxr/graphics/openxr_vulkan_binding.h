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

#include "vertexnova/xr/xr_error.h"

#include <openxr/openxr.h>

#include <cstdint>

#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
#include <vulkan/vulkan.h>
#endif

namespace vne::xr {

/** @brief Fallback Vulkan bootstrap via xrCreateVulkanInstance/DeviceKHR. */
class OpenXrVulkanBinding final {
   public:
    [[nodiscard]] Result createVulkanInstance(XrInstance instance,
                                              XrSystemId system_id,
                                              PFN_xrGetInstanceProcAddr get_proc,
                                              bool use_enable2);
    void destroy();

#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
    [[nodiscard]] VkInstance vkInstance() const;
    [[nodiscard]] VkPhysicalDevice vkPhysicalDevice() const;
    [[nodiscard]] VkDevice vkDevice() const;
#endif
    [[nodiscard]] std::uint32_t queueFamilyIndex() const { return queue_family_index_; }

   private:
#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
#endif
    std::uint32_t queue_family_index_ = 0;
    bool owns_device_ = false;
};

}  // namespace vne::xr
