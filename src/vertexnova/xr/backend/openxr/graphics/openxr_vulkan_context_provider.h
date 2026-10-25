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

#include "vertexnova/rhi/vulkan_context_provider.h"
#include "vertexnova/xr/xr_error.h"

#include <openxr/openxr.h>

#include <optional>
#include <string>
#include <vector>

namespace vne::xr {

/**
 * @brief Lets the OpenXR runtime create the Vulkan instance and device that vnerhi uses.
 *
 * @details With `XR_KHR_vulkan_enable2` the instance and device are created through
 * `xrCreateVulkanInstanceKHR` / `xrCreateVulkanDeviceKHR`, which add the runtime's own extensions. With the
 * legacy `XR_KHR_vulkan_enable` the provider asks vnerhi to enable the names from
 * `xrGetVulkanInstanceExtensionsKHR` / `xrGetVulkanDeviceExtensionsKHR` and calls `vkCreateInstance` /
 * `vkCreateDevice` itself. In both cases the GPU is the one the runtime selects.
 *
 * `prepare()` must run before the Vulkan instance is created: the OpenXR specification requires the graphics
 * requirements query to precede it.
 */
class OpenXrVulkanContextProvider final : public vne::rhi::vulkan::IVulkanContextProvider {
   public:
    OpenXrVulkanContextProvider() = default;

    OpenXrVulkanContextProvider(const OpenXrVulkanContextProvider&) = delete;
    OpenXrVulkanContextProvider& operator=(const OpenXrVulkanContextProvider&) = delete;

    /**
     * @brief Resolve the OpenXR entry points and query the runtime's Vulkan requirements.
     * @return `eErrorNotSupported` when the runtime lacks the Vulkan entry points.
     */
    [[nodiscard]] Result prepare(XrInstance instance,
                                 XrSystemId system_id,
                                 PFN_xrGetInstanceProcAddr get_proc,
                                 bool use_enable2);

    [[nodiscard]] bool usesEnable2() const noexcept { return use_enable2_; }

    // ----- vne::rhi::vulkan::IVulkanContextProvider -----
    [[nodiscard]] std::int32_t createInstance(const VkInstanceCreateInfo& create_info,
                                              VkInstance_T** out_instance) override;
    [[nodiscard]] std::int32_t selectPhysicalDevice(VkInstance_T* instance,
                                                    VkPhysicalDevice_T** out_physical_device) override;
    [[nodiscard]] std::int32_t createDevice(VkPhysicalDevice_T* physical_device,
                                            const VkDeviceCreateInfo& create_info,
                                            VkDevice_T** out_device) override;

    /** @brief The runtime's lowest supported Vulkan version, from the graphics requirements query. */
    [[nodiscard]] std::optional<vne::rhi::ApiVersion> getMinApiVersion() const override { return min_api_version_; }
    [[nodiscard]] std::vector<std::string> getInstanceExtensions() const override { return instance_extensions_; }
    [[nodiscard]] std::vector<std::string> getDeviceExtensions() const override { return device_extensions_; }

   private:
    /** Legacy enable only: read the space-separated extension lists the runtime requires. */
    void collectLegacyExtensions();

    XrInstance xr_instance_ = XR_NULL_HANDLE;
    XrSystemId system_id_ = XR_NULL_SYSTEM_ID;
    PFN_xrGetInstanceProcAddr get_proc_ = nullptr;
    bool use_enable2_ = false;

    std::optional<vne::rhi::ApiVersion> min_api_version_;
    std::vector<std::string> instance_extensions_;
    std::vector<std::string> device_extensions_;
};

}  // namespace vne::xr
