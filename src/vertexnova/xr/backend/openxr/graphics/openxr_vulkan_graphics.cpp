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

#include "vertexnova/xr/backend/openxr/graphics/openxr_vulkan_graphics.h"

#include <openxr/openxr.h>

#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
#include <openxr/openxr_platform.h>
#endif

#include "vertexnova/logging/logging.h"
#include "vertexnova/rhi/backend/vulkan/vulkan_device.h"
#include "vertexnova/rhi/graphics_factory.h"
#include "vertexnova/xr/backend/openxr/openxr_util.h"

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.xr.openxr.graphics");
}

namespace vne::xr {

OpenXrVulkanGraphics::~OpenXrVulkanGraphics() {
    shutdown();
}

Result OpenXrVulkanGraphics::initialize(Attribs attribs) {
    if (attribs.instance == XR_NULL_HANDLE || attribs.system_id == XR_NULL_SYSTEM_ID) {
        return Result::eErrorInvalidArgument;
    }
    attribs_ = attribs;
    use_vulkan_enable2_ = attribs.use_vulkan_enable2;
    return Result::eSuccess;
}

Result OpenXrVulkanGraphics::createDevice() {
#if !defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
    return Result::eErrorNotSupported;
#else
    if (attribs_.instance == XR_NULL_HANDLE) {
        return Result::eErrorBadState;
    }

    const auto backend = vne::rhi::GraphicsFactory::getDefaultBackend();
    auto physical = vne::rhi::GraphicsFactory::createPhysicalDevice(backend);
    if (!physical) {
        VNE_LOG_ERROR << "OpenXR: failed to create physical device";
        return Result::eErrorInitializationFailed;
    }

    vne::rhi::PhysicalDeviceDescriptor phys_desc{};
    const auto device_info = physical->getDefaultDevice(phys_desc);
    if (device_info.name.empty()) {
        VNE_LOG_ERROR << "OpenXR: no suitable Vulkan device";
        return Result::eErrorInitializationFailed;
    }

    vne::rhi::DeviceDescriptor dev_desc{};
    dev_desc.debug_name = "openxr_vulkan";
    device_ = physical->createDevice(device_info, dev_desc);
    if (!device_ || !device_->isValid()) {
        VNE_LOG_ERROR << "OpenXR: failed to create Vulkan logical device";
        return Result::eErrorInitializationFailed;
    }

    if (use_vulkan_enable2_) {
        auto pfn_get_requirements = reinterpret_cast<PFN_xrGetVulkanGraphicsRequirements2KHR>(
            attribs_.get_instance_proc_addr(attribs_.instance, "xrGetVulkanGraphicsRequirements2KHR"));
        if (pfn_get_requirements != nullptr) {
            XrVulkanGraphicsRequirements2KHR requirements{XR_TYPE_VULKAN_GRAPHICS_REQUIREMENTS2_KHR};
            const XrResult req_result = pfn_get_requirements(attribs_.instance, attribs_.system_id, &requirements);
            if (XR_FAILED(req_result)) {
                VNE_LOG_WARN << "OpenXR: xrGetVulkanGraphicsRequirements2KHR failed";
            }
        }
    } else {
        auto pfn_get_requirements = reinterpret_cast<PFN_xrGetVulkanGraphicsRequirementsKHR>(
            attribs_.get_instance_proc_addr(attribs_.instance, "xrGetVulkanGraphicsRequirementsKHR"));
        if (pfn_get_requirements != nullptr) {
            XrVulkanGraphicsRequirementsKHR requirements{XR_TYPE_VULKAN_GRAPHICS_REQUIREMENTS_KHR};
            const XrResult req_result = pfn_get_requirements(attribs_.instance, attribs_.system_id, &requirements);
            if (XR_FAILED(req_result)) {
                VNE_LOG_WARN << "OpenXR: xrGetVulkanGraphicsRequirementsKHR failed";
            }
        }
    }

    VNE_LOG_INFO << "OpenXR Vulkan device ready (enable2=" << use_vulkan_enable2_ << ")";
    return Result::eSuccess;
#endif
}

XrGraphicsBindingVulkanKHR OpenXrVulkanGraphics::getGraphicsBinding() const {
    XrGraphicsBindingVulkanKHR binding{XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR};
#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
    if (device_) {
        if (auto* vk_device = dynamic_cast<vne::rhi::VulkanDevice*>(device_.get())) {
            binding.instance = static_cast<VkInstance>(vk_device->getVulkanInstance());
            binding.physicalDevice = static_cast<VkPhysicalDevice>(vk_device->getVulkanPhysicalDevice());
            binding.device = static_cast<VkDevice>(vk_device->getVulkanDevice());
            binding.queueFamilyIndex = vk_device->getVulkanGraphicsQueueFamily();
            binding.queueIndex = 0;
        }
    }
#endif
    return binding;
}

void OpenXrVulkanGraphics::shutdown() {
    if (device_) {
        device_->shutdown();
        device_.reset();
    }
}

}  // namespace vne::xr
