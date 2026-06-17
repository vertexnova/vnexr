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

#include <memory>

namespace vne::rhi {
class IDevice;
}

namespace vne::xr {

/** @brief OpenXR Vulkan graphics binding via vnerhi (Diligent OpenXRAttribs pattern). */
class OpenXrVulkanGraphics final {
   public:
    struct Attribs {
        XrInstance instance = XR_NULL_HANDLE;
        XrSystemId system_id = XR_NULL_SYSTEM_ID;
        PFN_xrGetInstanceProcAddr get_instance_proc_addr = nullptr;
        bool use_vulkan_enable2 = false;
    };

    OpenXrVulkanGraphics() = default;
    ~OpenXrVulkanGraphics();

    OpenXrVulkanGraphics(const OpenXrVulkanGraphics&) = delete;
    OpenXrVulkanGraphics& operator=(const OpenXrVulkanGraphics&) = delete;

    [[nodiscard]] Result initialize(Attribs attribs);
    [[nodiscard]] Result createDevice();
    [[nodiscard]] XrGraphicsBindingVulkanKHR getGraphicsBinding() const;
    [[nodiscard]] std::shared_ptr<vne::rhi::IDevice> getDevice() const { return device_; }
    [[nodiscard]] bool usesVulkanEnable2() const { return use_vulkan_enable2_; }
    void shutdown();

   private:
    Attribs attribs_{};
    bool use_vulkan_enable2_ = false;
    std::shared_ptr<vne::rhi::IDevice> device_;
};

}  // namespace vne::xr
