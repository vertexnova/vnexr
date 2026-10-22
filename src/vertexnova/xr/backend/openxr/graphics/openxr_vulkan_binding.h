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

#include "vertexnova/rhi/physical_device.h"
#include "vertexnova/xr/xr_error.h"

#include <openxr/openxr.h>

#include <cstdint>
#include <string>
#include <vector>

namespace vne::xr {

/**
 * @brief Resolves OpenXR Vulkan enable2 (or enable) entry points into vnerhi `VulkanCreateHooks`.
 *
 * @details Does not create a Vulkan device itself. vnerhi builds queue/feature info and calls these
 * hooks instead of `vkCreateInstance` / `vkCreateDevice`.
 */
class OpenXrVulkanBinding final {
   public:
    OpenXrVulkanBinding() = default;
    ~OpenXrVulkanBinding() = default;

    OpenXrVulkanBinding(const OpenXrVulkanBinding&) = delete;
    OpenXrVulkanBinding& operator=(const OpenXrVulkanBinding&) = delete;

    [[nodiscard]] Result prepare(XrInstance instance,
                                 XrSystemId system_id,
                                 PFN_xrGetInstanceProcAddr get_proc,
                                 bool use_enable2);

    /** @brief Hooks for `PhysicalDeviceDescriptor` / `DeviceDescriptor` (valid until next prepare). */
    [[nodiscard]] vne::rhi::VulkanCreateHooks hooks() const;

    [[nodiscard]] bool usesEnable2() const { return use_enable2_; }

   private:
#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
    static std::int32_t createInstanceHook(void* user_data, const void* vk_instance_create_info, void** out_instance);
    static std::int32_t selectPhysicalDeviceHook(void* user_data, void* vk_instance, void** out_physical_device);
    static std::int32_t createDeviceHook(void* user_data,
                                         void* vk_physical_device,
                                         const void* vk_device_create_info,
                                         void** out_device);

    void collectExtraExtensions();
#endif

    XrInstance xr_instance_ = XR_NULL_HANDLE;
    XrSystemId system_id_ = XR_NULL_SYSTEM_ID;
    PFN_xrGetInstanceProcAddr get_proc_ = nullptr;
    bool use_enable2_ = false;
    bool prepared_ = false;

    std::vector<const char*> extra_instance_extensions_;
    std::vector<const char*> extra_device_extensions_;
    std::vector<std::string> extra_instance_extension_storage_;
    std::vector<std::string> extra_device_extension_storage_;
};

}  // namespace vne::xr
