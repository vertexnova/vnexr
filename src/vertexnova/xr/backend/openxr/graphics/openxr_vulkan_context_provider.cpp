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

#include "vertexnova/xr/backend/openxr/graphics/openxr_vulkan_context_provider.h"

#include "vertexnova/logging/logging.h"
#include "vertexnova/rhi/rhi_constants.h"
#include "vertexnova/xr/backend/openxr/openxr_platform_prelude.h"
#include "vertexnova/xr/backend/openxr/openxr_util.h"

#include <sstream>
#include <utility>

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.xr.openxr.vulkan_context");

[[nodiscard]] std::int32_t toInt(VkResult result) noexcept {
    return static_cast<std::int32_t>(result);
}
}  // namespace

namespace vne::xr {

namespace {

[[nodiscard]] std::vector<std::string> splitNames(const std::vector<char>& buffer) {
    std::vector<std::string> names;
    std::istringstream stream(buffer.data());
    std::string token;
    while (stream >> token) {
        names.push_back(std::move(token));
    }
    return names;
}

/** Reads a space-separated name list through a two-call OpenXR query. */
template<typename Pfn>
[[nodiscard]] std::vector<std::string> queryNames(Pfn pfn, XrInstance instance, XrSystemId system_id) {
    if (pfn == nullptr) {
        return {};
    }
    std::uint32_t capacity = 0;
    if (XR_FAILED(pfn(instance, system_id, 0, &capacity, nullptr)) || capacity == 0) {
        return {};
    }
    std::vector<char> buffer(capacity);
    if (XR_FAILED(pfn(instance, system_id, capacity, &capacity, buffer.data()))) {
        return {};
    }
    return splitNames(buffer);
}

}  // namespace

Result OpenXrVulkanContextProvider::prepare(XrInstance instance,
                                            XrSystemId system_id,
                                            PFN_xrGetInstanceProcAddr get_proc,
                                            bool use_enable2) {
    if (instance == XR_NULL_HANDLE || system_id == XR_NULL_SYSTEM_ID || get_proc == nullptr) {
        return Result::eErrorInvalidArgument;
    }
    xr_instance_ = instance;
    system_id_ = system_id;
    get_proc_ = get_proc;
    use_enable2_ = use_enable2;

    const auto load_all_present = [&](std::initializer_list<const char*> names) {
        for (const char* name : names) {
            if (OpenXrUtil::loadXrProc<PFN_xrVoidFunction>(instance, get_proc, name) == nullptr) {
                VNE_LOG_ERROR << "OpenXR entry point missing: " << name;
                return false;
            }
        }
        return true;
    };
    const bool entry_points_ok = use_enable2 ? load_all_present({"xrCreateVulkanInstanceKHR",
                                                                 "xrCreateVulkanDeviceKHR",
                                                                 "xrGetVulkanGraphicsDevice2KHR",
                                                                 "xrGetVulkanGraphicsRequirements2KHR"})
                                             : load_all_present({"xrGetVulkanGraphicsDeviceKHR",
                                                                 "xrGetVulkanGraphicsRequirementsKHR",
                                                                 "xrGetVulkanInstanceExtensionsKHR",
                                                                 "xrGetVulkanDeviceExtensionsKHR"});
    if (!entry_points_ok) {
        return Result::eErrorNotSupported;
    }

    // The specification requires this query before any Vulkan object is created for the session.
    XrGraphicsRequirementsVulkanKHR requirements{use_enable2 ? XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR
                                                             : XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR};
    XrResult requirements_result = XR_ERROR_FUNCTION_UNSUPPORTED;
    if (use_enable2) {
        auto pfn =
            OpenXrUtil::loadXrProc<PFN_xrGetVulkanGraphicsRequirements2KHR>(instance,
                                                                            get_proc,
                                                                            "xrGetVulkanGraphicsRequirements2KHR");
        requirements_result = pfn(instance, system_id, &requirements);
    } else {
        auto pfn = OpenXrUtil::loadXrProc<PFN_xrGetVulkanGraphicsRequirementsKHR>(instance,
                                                                                  get_proc,
                                                                                  "xrGetVulkanGraphicsRequirementsKHR");
        requirements_result = pfn(instance, system_id, &requirements);
    }
    if (XR_FAILED(requirements_result)) {
        VNE_LOG_ERROR << "OpenXR Vulkan graphics requirements query failed (XrResult=" << requirements_result << ")";
        return Result::eErrorInitializationFailed;
    }

    min_api_version_ = vne::rhi::ApiVersion{XR_VERSION_MAJOR(requirements.minApiVersionSupported),
                                            XR_VERSION_MINOR(requirements.minApiVersionSupported),
                                            0};
    // maxApiVersionSupported is the newest version the runtime was tested with, not a limit; warn only.
    const vne::rhi::ApiVersion tested_max{XR_VERSION_MAJOR(requirements.maxApiVersionSupported),
                                          XR_VERSION_MINOR(requirements.maxApiVersionSupported),
                                          0};
    if (vne::rhi::kVulkanVersionMajorDefault > tested_max.major
        || (vne::rhi::kVulkanVersionMajorDefault == tested_max.major
            && vne::rhi::kVulkanVersionMinorDefault > tested_max.minor)) {
        VNE_LOG_WARN << "OpenXR runtime was tested with Vulkan up to " << tested_max.toString() << "; the RHI requests "
                     << vne::rhi::kVulkanVersionMajorDefault << "." << vne::rhi::kVulkanVersionMinorDefault
                     << " when the loader offers it";
    }

    if (!use_enable2) {
        collectLegacyExtensions();
    }
    VNE_LOG_INFO << "OpenXR Vulkan context ready (enable2=" << use_enable2_
                 << ", min Vulkan=" << min_api_version_->toString()
                 << ", instance extensions=" << instance_extensions_.size()
                 << ", device extensions=" << device_extensions_.size() << ")";
    return Result::eSuccess;
}

void OpenXrVulkanContextProvider::collectLegacyExtensions() {
    instance_extensions_ =
        queryNames(OpenXrUtil::loadXrProc<PFN_xrGetVulkanInstanceExtensionsKHR>(xr_instance_,
                                                                                get_proc_,
                                                                                "xrGetVulkanInstanceExtensionsKHR"),
                   xr_instance_,
                   system_id_);
    device_extensions_ =
        queryNames(OpenXrUtil::loadXrProc<PFN_xrGetVulkanDeviceExtensionsKHR>(xr_instance_,
                                                                              get_proc_,
                                                                              "xrGetVulkanDeviceExtensionsKHR"),
                   xr_instance_,
                   system_id_);
}

std::int32_t OpenXrVulkanContextProvider::createInstance(const VkInstanceCreateInfo& create_info,
                                                         VkInstance_T** out_instance) {
    if (xr_instance_ == XR_NULL_HANDLE || out_instance == nullptr) {
        return toInt(VK_ERROR_INITIALIZATION_FAILED);
    }
    if (!use_enable2_) {
        // vnerhi has already merged the runtime's required extensions into create_info.
        return toInt(vkCreateInstance(&create_info, nullptr, out_instance));
    }

    auto pfn =
        OpenXrUtil::loadXrProc<PFN_xrCreateVulkanInstanceKHR>(xr_instance_, get_proc_, "xrCreateVulkanInstanceKHR");
    if (pfn == nullptr) {
        return toInt(VK_ERROR_EXTENSION_NOT_PRESENT);
    }
    XrVulkanInstanceCreateInfoKHR xr_info{XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR};
    xr_info.systemId = system_id_;
    xr_info.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
    xr_info.vulkanCreateInfo = &create_info;

    VkResult vk_result = VK_ERROR_UNKNOWN;
    if (XR_FAILED(pfn(xr_instance_, &xr_info, out_instance, &vk_result))) {
        return toInt(VK_ERROR_INITIALIZATION_FAILED);
    }
    return toInt(vk_result);
}

std::int32_t OpenXrVulkanContextProvider::selectPhysicalDevice(VkInstance_T* instance,
                                                               VkPhysicalDevice_T** out_physical_device) {
    if (instance == nullptr || out_physical_device == nullptr) {
        return toInt(VK_ERROR_INITIALIZATION_FAILED);
    }
    if (use_enable2_) {
        auto pfn = OpenXrUtil::loadXrProc<PFN_xrGetVulkanGraphicsDevice2KHR>(xr_instance_,
                                                                             get_proc_,
                                                                             "xrGetVulkanGraphicsDevice2KHR");
        if (pfn == nullptr) {
            return toInt(VK_ERROR_EXTENSION_NOT_PRESENT);
        }
        XrVulkanGraphicsDeviceGetInfoKHR get_info{XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR};
        get_info.systemId = system_id_;
        get_info.vulkanInstance = instance;
        return XR_FAILED(pfn(xr_instance_, &get_info, out_physical_device)) ? toInt(VK_ERROR_INITIALIZATION_FAILED)
                                                                            : toInt(VK_SUCCESS);
    }
    auto pfn = OpenXrUtil::loadXrProc<PFN_xrGetVulkanGraphicsDeviceKHR>(xr_instance_,
                                                                        get_proc_,
                                                                        "xrGetVulkanGraphicsDeviceKHR");
    if (pfn == nullptr) {
        return toInt(VK_ERROR_EXTENSION_NOT_PRESENT);
    }
    return XR_FAILED(pfn(xr_instance_, system_id_, instance, out_physical_device))
               ? toInt(VK_ERROR_INITIALIZATION_FAILED)
               : toInt(VK_SUCCESS);
}

std::int32_t OpenXrVulkanContextProvider::createDevice(VkPhysicalDevice_T* physical_device,
                                                       const VkDeviceCreateInfo& create_info,
                                                       VkDevice_T** out_device) {
    if (physical_device == nullptr || out_device == nullptr) {
        return toInt(VK_ERROR_INITIALIZATION_FAILED);
    }
    if (!use_enable2_) {
        // vnerhi has already merged the runtime's required extensions into create_info.
        return toInt(vkCreateDevice(physical_device, &create_info, nullptr, out_device));
    }

    auto pfn = OpenXrUtil::loadXrProc<PFN_xrCreateVulkanDeviceKHR>(xr_instance_, get_proc_, "xrCreateVulkanDeviceKHR");
    if (pfn == nullptr) {
        return toInt(VK_ERROR_EXTENSION_NOT_PRESENT);
    }
    XrVulkanDeviceCreateInfoKHR xr_info{XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR};
    xr_info.systemId = system_id_;
    xr_info.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
    xr_info.vulkanPhysicalDevice = physical_device;
    xr_info.vulkanCreateInfo = &create_info;

    VkResult vk_result = VK_ERROR_UNKNOWN;
    if (XR_FAILED(pfn(xr_instance_, &xr_info, out_device, &vk_result))) {
        return toInt(VK_ERROR_INITIALIZATION_FAILED);
    }
    return toInt(vk_result);
}

}  // namespace vne::xr
