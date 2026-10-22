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

#include "vertexnova/logging/logging.h"
#include "vertexnova/xr/backend/openxr/openxr_util.h"

#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
#include <openxr/openxr_platform.h>
#include <vulkan/vulkan.h>

#include <sstream>
#endif

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.xr.openxr.vulkan_binding");
}

namespace vne::xr {

#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)

namespace {

void splitExtensionString(const char* extensions, std::vector<std::string>& out_storage, std::vector<const char*>& out_ptrs) {
    out_storage.clear();
    out_ptrs.clear();
    if (extensions == nullptr || extensions[0] == '\0') {
        return;
    }
    std::istringstream stream(extensions);
    std::string token;
    while (stream >> token) {
        out_storage.push_back(std::move(token));
    }
    out_ptrs.reserve(out_storage.size());
    for (const auto& name : out_storage) {
        out_ptrs.push_back(name.c_str());
    }
}

}  // namespace

void OpenXrVulkanBinding::collectExtraExtensions() {
    extra_instance_extension_storage_.clear();
    extra_device_extension_storage_.clear();
    extra_instance_extensions_.clear();
    extra_device_extensions_.clear();

    // Legacy enable enumerates required Vulkan extensions as space-separated strings.
    // enable2 merges them inside xrCreateVulkan*KHR; still request them when available.
    auto pfn_inst_ext = OpenXrUtil::loadXrProc<PFN_xrGetVulkanInstanceExtensionsKHR>(
        xr_instance_, get_proc_, "xrGetVulkanInstanceExtensionsKHR");
    auto pfn_dev_ext = OpenXrUtil::loadXrProc<PFN_xrGetVulkanDeviceExtensionsKHR>(
        xr_instance_, get_proc_, "xrGetVulkanDeviceExtensionsKHR");

    if (pfn_inst_ext != nullptr) {
        uint32_t capacity = 0;
        if (XR_SUCCEEDED(pfn_inst_ext(xr_instance_, system_id_, 0, &capacity, nullptr)) && capacity > 0) {
            std::vector<char> buffer(capacity);
            if (XR_SUCCEEDED(pfn_inst_ext(xr_instance_, system_id_, capacity, &capacity, buffer.data()))) {
                splitExtensionString(buffer.data(), extra_instance_extension_storage_, extra_instance_extensions_);
            }
        }
    }
    if (pfn_dev_ext != nullptr) {
        uint32_t capacity = 0;
        if (XR_SUCCEEDED(pfn_dev_ext(xr_instance_, system_id_, 0, &capacity, nullptr)) && capacity > 0) {
            std::vector<char> buffer(capacity);
            if (XR_SUCCEEDED(pfn_dev_ext(xr_instance_, system_id_, capacity, &capacity, buffer.data()))) {
                splitExtensionString(buffer.data(), extra_device_extension_storage_, extra_device_extensions_);
            }
        }
    }
}

std::int32_t OpenXrVulkanBinding::createInstanceHook(void* user_data,
                                                     const void* vk_instance_create_info,
                                                     void** out_instance) {
    auto* self = static_cast<OpenXrVulkanBinding*>(user_data);
    if (self == nullptr || out_instance == nullptr || vk_instance_create_info == nullptr) {
        return static_cast<std::int32_t>(VK_ERROR_INITIALIZATION_FAILED);
    }

    auto pfn = OpenXrUtil::loadXrProc<PFN_xrCreateVulkanInstanceKHR>(
        self->xr_instance_, self->get_proc_, "xrCreateVulkanInstanceKHR");
    if (pfn == nullptr) {
        return static_cast<std::int32_t>(VK_ERROR_EXTENSION_NOT_PRESENT);
    }

    XrVulkanInstanceCreateInfoKHR create_info{XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR};
    create_info.systemId = self->system_id_;
    create_info.createFlags = 0;
    create_info.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
    create_info.vulkanCreateInfo = static_cast<const VkInstanceCreateInfo*>(vk_instance_create_info);
    create_info.vulkanAllocator = nullptr;

    VkInstance vk_instance = VK_NULL_HANDLE;
    VkResult vk_result = VK_ERROR_UNKNOWN;
    if (XR_FAILED(pfn(self->xr_instance_, &create_info, &vk_instance, &vk_result))) {
        return static_cast<std::int32_t>(VK_ERROR_INITIALIZATION_FAILED);
    }
    *out_instance = vk_instance;
    return static_cast<std::int32_t>(vk_result);
}

std::int32_t OpenXrVulkanBinding::selectPhysicalDeviceHook(void* user_data,
                                                           void* vk_instance,
                                                           void** out_physical_device) {
    auto* self = static_cast<OpenXrVulkanBinding*>(user_data);
    if (self == nullptr || out_physical_device == nullptr || vk_instance == nullptr) {
        return static_cast<std::int32_t>(VK_ERROR_INITIALIZATION_FAILED);
    }

    VkPhysicalDevice physical = VK_NULL_HANDLE;
    if (self->use_enable2_) {
        auto pfn = OpenXrUtil::loadXrProc<PFN_xrGetVulkanGraphicsDevice2KHR>(
            self->xr_instance_, self->get_proc_, "xrGetVulkanGraphicsDevice2KHR");
        if (pfn == nullptr) {
            return static_cast<std::int32_t>(VK_ERROR_EXTENSION_NOT_PRESENT);
        }
        XrVulkanGraphicsDeviceGetInfoKHR get_info{XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR};
        get_info.systemId = self->system_id_;
        get_info.vulkanInstance = static_cast<VkInstance>(vk_instance);
        if (XR_FAILED(pfn(self->xr_instance_, &get_info, &physical))) {
            return static_cast<std::int32_t>(VK_ERROR_INITIALIZATION_FAILED);
        }
    } else {
        auto pfn = OpenXrUtil::loadXrProc<PFN_xrGetVulkanGraphicsDeviceKHR>(
            self->xr_instance_, self->get_proc_, "xrGetVulkanGraphicsDeviceKHR");
        if (pfn == nullptr) {
            return static_cast<std::int32_t>(VK_ERROR_EXTENSION_NOT_PRESENT);
        }
        if (XR_FAILED(pfn(self->xr_instance_,
                          self->system_id_,
                          static_cast<VkInstance>(vk_instance),
                          &physical))) {
            return static_cast<std::int32_t>(VK_ERROR_INITIALIZATION_FAILED);
        }
    }

    *out_physical_device = physical;
    return static_cast<std::int32_t>(VK_SUCCESS);
}

std::int32_t OpenXrVulkanBinding::createDeviceHook(void* user_data,
                                                   void* vk_physical_device,
                                                   const void* vk_device_create_info,
                                                   void** out_device) {
    auto* self = static_cast<OpenXrVulkanBinding*>(user_data);
    if (self == nullptr || out_device == nullptr || vk_physical_device == nullptr || vk_device_create_info == nullptr) {
        return static_cast<std::int32_t>(VK_ERROR_INITIALIZATION_FAILED);
    }

    auto pfn =
        OpenXrUtil::loadXrProc<PFN_xrCreateVulkanDeviceKHR>(self->xr_instance_, self->get_proc_, "xrCreateVulkanDeviceKHR");
    if (pfn == nullptr) {
        return static_cast<std::int32_t>(VK_ERROR_EXTENSION_NOT_PRESENT);
    }

    XrVulkanDeviceCreateInfoKHR device_info{XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR};
    device_info.systemId = self->system_id_;
    device_info.createFlags = 0;
    device_info.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
    device_info.vulkanPhysicalDevice = static_cast<VkPhysicalDevice>(vk_physical_device);
    device_info.vulkanCreateInfo = static_cast<const VkDeviceCreateInfo*>(vk_device_create_info);
    device_info.vulkanAllocator = nullptr;

    VkDevice vk_device = VK_NULL_HANDLE;
    VkResult vk_result = VK_ERROR_UNKNOWN;
    if (XR_FAILED(pfn(self->xr_instance_, &device_info, &vk_device, &vk_result))) {
        return static_cast<std::int32_t>(VK_ERROR_INITIALIZATION_FAILED);
    }
    *out_device = vk_device;
    return static_cast<std::int32_t>(vk_result);
}

#endif  // VNE_XR_OPENXR_GRAPHICS_VULKAN

Result OpenXrVulkanBinding::prepare(XrInstance instance,
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
    if (instance == XR_NULL_HANDLE || system_id == XR_NULL_SYSTEM_ID || get_proc == nullptr) {
        return Result::eErrorInvalidArgument;
    }

    xr_instance_ = instance;
    system_id_ = system_id;
    get_proc_ = get_proc;
    use_enable2_ = use_enable2;

    auto pfn_create_instance =
        OpenXrUtil::loadXrProc<PFN_xrCreateVulkanInstanceKHR>(instance, get_proc, "xrCreateVulkanInstanceKHR");
    auto pfn_create_device =
        OpenXrUtil::loadXrProc<PFN_xrCreateVulkanDeviceKHR>(instance, get_proc, "xrCreateVulkanDeviceKHR");
    if (pfn_create_instance == nullptr || pfn_create_device == nullptr) {
        VNE_LOG_ERROR << "OpenXR Vulkan create instance/device entry points missing";
        return Result::eErrorNotSupported;
    }

    if (use_enable2) {
        auto pfn_get_device =
            OpenXrUtil::loadXrProc<PFN_xrGetVulkanGraphicsDevice2KHR>(instance, get_proc, "xrGetVulkanGraphicsDevice2KHR");
        if (pfn_get_device == nullptr) {
            VNE_LOG_ERROR << "xrGetVulkanGraphicsDevice2KHR missing";
            return Result::eErrorNotSupported;
        }
    } else {
        auto pfn_get_device =
            OpenXrUtil::loadXrProc<PFN_xrGetVulkanGraphicsDeviceKHR>(instance, get_proc, "xrGetVulkanGraphicsDeviceKHR");
        if (pfn_get_device == nullptr) {
            VNE_LOG_ERROR << "xrGetVulkanGraphicsDeviceKHR missing";
            return Result::eErrorNotSupported;
        }
    }

    collectExtraExtensions();
    prepared_ = true;
    VNE_LOG_INFO << "OpenXR Vulkan binding hooks ready (enable2=" << use_enable2_ << ", inst_ext="
                 << extra_instance_extensions_.size() << ", dev_ext=" << extra_device_extensions_.size() << ")";
    return Result::eSuccess;
#endif
}

vne::rhi::VulkanCreateHooks OpenXrVulkanBinding::hooks() const {
    vne::rhi::VulkanCreateHooks result{};
#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
    if (!prepared_) {
        return result;
    }
    result.create_instance = &OpenXrVulkanBinding::createInstanceHook;
    result.select_physical_device = &OpenXrVulkanBinding::selectPhysicalDeviceHook;
    result.create_device = &OpenXrVulkanBinding::createDeviceHook;
    result.user_data = const_cast<OpenXrVulkanBinding*>(this);
    if (!extra_instance_extensions_.empty()) {
        result.extra_instance_extensions = extra_instance_extensions_.data();
        result.extra_instance_extension_count = static_cast<std::uint32_t>(extra_instance_extensions_.size());
    }
    if (!extra_device_extensions_.empty()) {
        result.extra_device_extensions = extra_device_extensions_.data();
        result.extra_device_extension_count = static_cast<std::uint32_t>(extra_device_extensions_.size());
    }
#endif
    return result;
}

}  // namespace vne::xr
