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

#include "vertexnova/xr/backend/openxr/graphics/openxr_vulkan_binding.h"

#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
#include <openxr/openxr_platform.h>
#include <vulkan/vulkan.h>
#endif

namespace vne::xr {

Result OpenXrVulkanBinding::createVulkanInstance(XrInstance instance,
                                                 XrSystemId system_id,
                                                 PFN_xrGetInstanceProcAddr get_proc,
                                                 bool use_enable2) {
#if !defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
    (void)instance;
    (void)system_id;
    (void)get_proc;
    (void)use_enable2;
    return Result::eErrorNotSupported;
#else
    (void)use_enable2;
    auto pfn_create_instance =
        reinterpret_cast<PFN_xrCreateVulkanInstanceKHR>(get_proc(instance, "xrCreateVulkanInstanceKHR"));
    auto pfn_create_device =
        reinterpret_cast<PFN_xrCreateVulkanDeviceKHR>(get_proc(instance, "xrCreateVulkanDeviceKHR"));
    if (pfn_create_instance == nullptr || pfn_create_device == nullptr) {
        return Result::eErrorNotSupported;
    }

    XrVulkanInstanceCreateInfoKHR create_info{XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR};
    VkInstanceCreateInfo vk_create_info{};
    create_info.systemId = system_id;
    create_info.createFlags = 0;
    create_info.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
    create_info.vulkanCreateInfo = &vk_create_info;
    create_info.vulkanAllocator = nullptr;

    VkInstance vk_instance = VK_NULL_HANDLE;
    if (XR_FAILED(pfn_create_instance(instance, &create_info, &vk_instance, &physical_device_))) {
        return Result::eErrorOpenXrFailed;
    }
    instance_ = vk_instance;

    XrVulkanDeviceCreateInfoKHR device_info{XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR};
    VkDeviceCreateInfo vk_device_info{};
    device_info.systemId = system_id;
    device_info.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
    device_info.vulkanCreateInfo = &vk_device_info;
    device_info.vulkanPhysicalDevice = physical_device_;
    device_info.vulkanAllocator = nullptr;

    VkDevice vk_device = VK_NULL_HANDLE;
    if (XR_FAILED(pfn_create_device(instance, &device_info, &vk_device))) {
        return Result::eErrorOpenXrFailed;
    }
    device_ = vk_device;
    queue_family_index_ = 0;
    owns_device_ = true;
    return Result::eSuccess;
#endif
}

void OpenXrVulkanBinding::destroy() {
#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
    if (owns_device_) {
        if (device_ != VK_NULL_HANDLE) {
            vkDestroyDevice(device_, nullptr);
            device_ = VK_NULL_HANDLE;
        }
        if (instance_ != VK_NULL_HANDLE) {
            vkDestroyInstance(instance_, nullptr);
            instance_ = VK_NULL_HANDLE;
        }
        physical_device_ = VK_NULL_HANDLE;
        owns_device_ = false;
    }
#endif
}

VkInstance OpenXrVulkanBinding::vkInstance() const {
#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
    return instance_;
#else
    return VK_NULL_HANDLE;
#endif
}

VkPhysicalDevice OpenXrVulkanBinding::vkPhysicalDevice() const {
#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
    return physical_device_;
#else
    return VK_NULL_HANDLE;
#endif
}

VkDevice OpenXrVulkanBinding::vkDevice() const {
#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
    return device_;
#else
    return VK_NULL_HANDLE;
#endif
}

}  // namespace vne::xr
