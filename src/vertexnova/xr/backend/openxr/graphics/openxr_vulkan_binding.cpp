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

#include "vertexnova/xr/backend/openxr/openxr_util.h"

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
    auto pfn_create_instance =
        OpenXrUtil::loadXrProc<PFN_xrCreateVulkanInstanceKHR>(instance, get_proc, "xrCreateVulkanInstanceKHR");
    auto pfn_create_device =
        OpenXrUtil::loadXrProc<PFN_xrCreateVulkanDeviceKHR>(instance, get_proc, "xrCreateVulkanDeviceKHR");
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
    VkResult vk_result = VK_SUCCESS;
    if (XR_FAILED(pfn_create_instance(instance, &create_info, &vk_instance, &vk_result)) || vk_result != VK_SUCCESS) {
        return Result::eErrorOpenXrFailed;
    }
    instance_ = vk_instance;

    if (use_enable2) {
        auto pfn_get_device =
            OpenXrUtil::loadXrProc<PFN_xrGetVulkanGraphicsDevice2KHR>(instance, get_proc, "xrGetVulkanGraphicsDevice2KHR");
        if (pfn_get_device == nullptr) {
            destroy();
            return Result::eErrorNotSupported;
        }
        XrVulkanGraphicsDeviceGetInfoKHR get_info{XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR};
        get_info.systemId = system_id;
        get_info.vulkanInstance = instance_;
        if (XR_FAILED(pfn_get_device(instance, &get_info, &physical_device_))) {
            destroy();
            return Result::eErrorOpenXrFailed;
        }
    } else {
        auto pfn_get_device =
            OpenXrUtil::loadXrProc<PFN_xrGetVulkanGraphicsDeviceKHR>(instance, get_proc, "xrGetVulkanGraphicsDeviceKHR");
        if (pfn_get_device == nullptr) {
            destroy();
            return Result::eErrorNotSupported;
        }
        if (XR_FAILED(pfn_get_device(instance, system_id, instance_, &physical_device_))) {
            destroy();
            return Result::eErrorOpenXrFailed;
        }
    }

    XrVulkanDeviceCreateInfoKHR device_info{XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR};
    VkDeviceCreateInfo vk_device_info{};
    device_info.systemId = system_id;
    device_info.createFlags = 0;
    device_info.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
    device_info.vulkanCreateInfo = &vk_device_info;
    device_info.vulkanPhysicalDevice = physical_device_;
    device_info.vulkanAllocator = nullptr;

    VkDevice vk_device = VK_NULL_HANDLE;
    vk_result = VK_SUCCESS;
    if (XR_FAILED(pfn_create_device(instance, &device_info, &vk_device, &vk_result)) || vk_result != VK_SUCCESS) {
        destroy();
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
