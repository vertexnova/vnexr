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

#include "vertexnova/xr/backend/openxr/graphics/openxr_vulkan_graphics.h"
#include "vertexnova/xr/backend/openxr/input/openxr_input.h"
#include "vertexnova/xr/backend/openxr/presentation/openxr_composition_layer.h"
#include "vertexnova/xr/backend/openxr/presentation/openxr_swapchain_bridge.h"
#include "vertexnova/xr/session.h"

#include <openxr/openxr.h>

#include <array>
#include <vector>

namespace vne::xr {

/** @brief OpenXR session orchestrator (Android / Windows). */
class OpenXrSession final : public ISession {
   public:
    explicit OpenXrSession(SessionConfig config);
    ~OpenXrSession() override;

    [[nodiscard]] BackendType backendType() const override;
    [[nodiscard]] SessionState state() const override;
    [[nodiscard]] bool isSessionRunning() const override;
    bool pollEvents() override;
    bool beginFrame(Frame& out_frame) override;
    void endFrame(const Frame& frame, const LayerParams& layers) override;
    void pollInput(InputState& out_input) override;

   private:
    SessionConfig config_;
    SessionState state_ = SessionState::eUnknown;
    bool running_ = true;
    bool session_running_ = false;
    bool session_begun_ = false;

    XrInstance instance_ = XR_NULL_HANDLE;
    XrSession session_ = XR_NULL_HANDLE;
    XrSystemId system_id_ = XR_NULL_SYSTEM_ID;
    XrSpace reference_space_ = XR_NULL_HANDLE;
    XrViewConfigurationType view_configuration_ = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    XrEnvironmentBlendMode blend_mode_ = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    bool use_vulkan_enable2_ = false;

    std::uint64_t frame_index_ = 0;
    XrTime predicted_display_time_ = 0;
    float near_z_ = 0.1f;
    float far_z_ = 100.f;

    OpenXrVulkanGraphics graphics_;
    OpenXrSwapchainBridge swapchain_bridge_;
    OpenXrCompositionLayer composition_layer_;
    OpenXrInput input_;

    std::vector<XrViewConfigurationView> view_configs_;
    std::int64_t color_format_ = 0;
    std::int64_t depth_format_ = 0;
    bool swapchains_ready_ = false;

    bool initOpenXr();
    void shutdownOpenXr();
    bool ensureSwapchains();
    void flushGpu();
    void handleSessionStateChanged(XrSessionState xr_state);
};

}  // namespace vne::xr
