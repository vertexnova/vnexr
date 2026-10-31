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
#include "vertexnova/xr/backend/openxr/platform/openxr_debug.h"
#include "vertexnova/xr/backend/openxr/presentation/openxr_swapchain_bridge.h"
#include "vertexnova/xr/session.h"

#include <openxr/openxr.h>

#include <array>
#include <chrono>
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
    [[nodiscard]] bool requestExit() override;
    [[nodiscard]] std::shared_ptr<vne::rhi::IDevice> graphicsDevice() const override;

   private:
    static constexpr std::uint32_t kViewCapacity = 2;  //!< Stereo: one view per eye.
    using ViewArray = std::array<XrView, kViewCapacity>;

    SessionConfig config_;
    SessionState state_ = SessionState::eUnknown;
    bool running_ = true;
    bool session_running_ = false;
    bool session_begun_ = false;

    XrInstance instance_ = XR_NULL_HANDLE;
    XrSession session_ = XR_NULL_HANDLE;
    XrSystemId system_id_ = XR_NULL_SYSTEM_ID;
    XrSpace reference_space_ = XR_NULL_HANDLE;
    XrSpace view_space_ = XR_NULL_HANDLE;  //!< Head pose, located in `reference_space_` each frame.
    OpenXrDebugMessenger debug_messenger_;
    XrViewConfigurationType view_configuration_ = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    XrEnvironmentBlendMode blend_mode_ = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    bool use_vulkan_enable2_ = false;

    std::uint64_t frame_index_ = 0;
    XrTime predicted_display_time_ = 0;
    float near_z_ = kDefaultNearZ;
    float far_z_ = kDefaultFarZ;

    OpenXrVulkanGraphics graphics_;
    OpenXrSwapchainBridge swapchain_bridge_;
    OpenXrCompositionLayer composition_layer_;
    OpenXrInput input_;

    std::vector<XrViewConfigurationView> view_configs_;
    std::int64_t color_format_ = 0;
    bool swapchains_ready_ = false;
    /** Last tracked eye views: reused for a frame or two when the runtime briefly reports no valid pose. */
    ViewArray last_located_views_{};
    std::uint32_t last_located_view_count_ = 0;
    bool has_located_views_ = false;

    /** Set by REFERENCE_SPACE_CHANGE_PENDING; applied when a frame's display time reaches `space_change_time_`. */
    bool has_space_change_pending_ = false;
    XrTime space_change_time_ = 0;
    std::uint32_t reference_space_epoch_ = 0;
    /** XR_FB_composition_layer_settings was enabled at instance creation. */
    bool has_layer_settings_ = false;

    /** Counters that make a silent failure visible: logged once per second when any is non-zero. */
    struct FrameHealth {
        std::uint32_t frames = 0;
        std::uint32_t skipped_no_pose = 0;    //!< Frames ended with no layer: no pose was ever valid.
        std::uint32_t stale_pose_frames = 0;  //!< Frames drawn with the previous pose.
        std::uint32_t unrendered_frames = 0;  //!< Frames ended with no layer: the app did not draw them.
        std::uint32_t acquire_failures = 0;
        std::uint32_t swapchain_recreations = 0;
        std::uint32_t wait_frame_failures = 0;
        std::uint32_t end_frame_failures = 0;
    };
    FrameHealth health_{};
    std::chrono::steady_clock::time_point health_log_time_ = std::chrono::steady_clock::now();
    /** True from a successful swapchain acquire in beginFrame() until the images are released in endFrame(). */
    bool are_images_acquired_ = false;
    bool is_exit_requested_ = false;
    XrReferenceSpaceType reference_space_type_ = XR_REFERENCE_SPACE_TYPE_LOCAL;

    bool initOpenXr();
    void shutdownOpenXr();
    bool ensureSwapchains();
    void flushGpu();
    /**
     * Locate both eyes for @p display_time. A frame with a lost position (or no fix at all) reuses the last located
     * views so the display never blanks. @return False only when nothing has ever been tracked.
     */
    [[nodiscard]] bool locateViews(XrTime display_time, ViewArray& out_views, std::uint32_t& out_count);
    /** @brief Head pose in the reference space; false when the runtime has no valid pose for it. */
    [[nodiscard]] bool locateHead(XrTime display_time, Pose& out_pose) const;
    void handleSessionStateChanged(XrSessionState xr_state);
    void processPendingEvents();
    void releaseSwapchains();
    void logHealth();
    [[nodiscard]] std::vector<XrViewConfigurationView> scaledViewConfigs() const;
    [[nodiscard]] XrReferenceSpaceType chooseReferenceSpaceType() const;
};

}  // namespace vne::xr
