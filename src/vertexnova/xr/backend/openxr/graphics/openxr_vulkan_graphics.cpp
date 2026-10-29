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
    return context_provider_->prepare(attribs.instance,
                                      attribs.system_id,
                                      attribs.get_instance_proc_addr,
                                      attribs.use_vulkan_enable2);
}

Result OpenXrVulkanGraphics::createDevice() {
#if !defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
    return Result::eErrorNotSupported;
#else
    if (attribs_.instance == XR_NULL_HANDLE) {
        return Result::eErrorBadState;
    }

    auto physical = vne::rhi::GraphicsFactory::createPhysicalDevice(vne::rhi::GraphicsBackend::eVulkan);
    if (!physical) {
        VNE_LOG_ERROR << "OpenXR: failed to create physical device";
        return Result::eErrorInitializationFailed;
    }

    vne::rhi::PhysicalDeviceDescriptor phys_desc{};
    phys_desc.external_context = context_provider_;
    vne::rhi::DeviceDescriptor dev_desc{};
    dev_desc.name = "openxr_vulkan";

    const auto device_info = physical->selectBestDevice(phys_desc);
    if (!device_info.isValid()) {
        VNE_LOG_ERROR << "OpenXR: no suitable Vulkan device from runtime selection";
        return Result::eErrorInitializationFailed;
    }
    device_ = physical->createDevice(device_info, dev_desc);
    if (!device_ || !device_->isValid()) {
        VNE_LOG_ERROR << "OpenXR: failed to create the Vulkan logical device through the runtime";
        return Result::eErrorInitializationFailed;
    }

    VNE_LOG_INFO << "OpenXR Vulkan device ready (enable2=" << use_vulkan_enable2_ << ")";
    return Result::eSuccess;
#endif
}

XrGraphicsBindingVulkanKHR OpenXrVulkanGraphics::getGraphicsBinding() const {
    XrGraphicsBindingVulkanKHR binding{XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR};
    if (!device_ || device_->getBackend() != vne::rhi::GraphicsBackend::eVulkan) {
        return binding;
    }
    const auto* vulkan_device = static_cast<const vne::rhi::VulkanDevice*>(device_.get());
    if (!vulkan_device->getVulkanDevice()) {
        return binding;
    }
    binding.instance = vulkan_device->getVulkanInstance();
    binding.physicalDevice = vulkan_device->getVulkanPhysicalDevice();
    binding.device = vulkan_device->getVulkanDevice();
    binding.queueFamilyIndex = vulkan_device->getVulkanGraphicsQueueFamily();
    binding.queueIndex = 0;
    return binding;
}

void OpenXrVulkanGraphics::shutdown() {
    if (device_) {
        device_->shutdown();
        device_.reset();
    }
}

}  // namespace vne::xr
