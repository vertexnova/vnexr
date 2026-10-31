#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "app/sample_registry.h"
#include "interaction/input_edges.h"
#include "interaction/scene_navigator.h"
#include "launcher/controller_visuals_layer.h"
#include "launcher/launcher_layer.h"
#include "layers/xr_layer_stack.h"
#include "render/text_renderer.h"
#include "render/xr_frame_renderer.h"
#include "vertexnova/xr/session.h"

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace vne::xr::samples {

/**
 * @brief One XR session for the whole app, running whichever sample the user picks.
 *
 * @details The host creates the session, device and swapchains once. Samples come and go inside it: picking a card
 * in the in-headset launcher (or `requestSample()`) detaches the running sample's layers and attaches the new
 * ones at a frame boundary, with no OpenXR or Vulkan teardown. The left Menu button returns to the launcher.
 * Every sample gets scene navigation (grip to move, both grips to zoom, B/Y to reset) and controller visuals
 * without doing anything.
 */
class XrSampleHost {
   public:
    /** @brief Called at the top of every loop iteration; return false to quit. Pumps the platform's event queue. */
    using FramePump = std::function<bool(ISession&)>;

    XrSampleHost(const SampleRegistry& registry, SessionConfig config);
    ~XrSampleHost();

    XrSampleHost(const XrSampleHost&) = delete;
    XrSampleHost& operator=(const XrSampleHost&) = delete;

    void setFramePump(FramePump pump) { frame_pump_ = std::move(pump); }

    /** @brief Sample to run on start. Empty (the default) shows the launcher. */
    void setInitialSample(std::string name) { initial_sample_ = std::move(name); }

    /** @brief Switch to @p name at the next frame boundary. An empty or unknown name shows the launcher. */
    void requestSample(std::string_view name);
    /** @brief Stop the running sample and show the launcher at the next frame boundary. */
    void returnToLauncher() { is_launcher_requested_ = true; }
    /** @brief End the session and leave `run()`. */
    void requestExit() { is_quit_asked_ = true; }

    /** @brief Create the session and run until it ends. @return Process exit code. */
    int run();

   private:
    enum class Mode { eLauncher, eSample };

    void buildOverlays();
    void applyPendingChanges(const Frame& frame);
    void startSample(const SampleInfo& info);
    void stopSample();
    void showLauncher();
    void onLauncherSelect(const LauncherEntry& entry);
    /** Head pose is trustworthy for placing content: valid, and the frames are actually being shown. */
    [[nodiscard]] bool canPlace(const Frame& frame) const;
    void logHeadPose(const Frame& frame, const char* reason) const;
    /** Menu, B/Y, snap turn and grip navigation for this frame. */
    void handleControls(const Frame& frame, const InputState& input);
    /** Logs where the head points every few seconds: compare with the "placed" lines to see where content is. */
    void logHeadPoseIfDue(const Frame& frame);

    const SampleRegistry& registry_;
    SessionConfig config_;
    FramePump frame_pump_;
    std::string initial_sample_;

    std::unique_ptr<ISession> session_;
    XrFrameRenderer renderer_;
    /** Shared by every layer that draws text; created with the session's device, destroyed after the layers. */
    TextRenderer text_renderer_;
    SceneNavigator navigator_;
    InputEdges edges_;

    /** Drawn in world space on top of every sample: controller visuals and the launcher panel. */
    XrLayerStack world_stack_;
    ControllerVisualsLayer* controller_visuals_ = nullptr;
    LauncherLayer* launcher_ = nullptr;
    /** The running sample's layers, drawn in scene space. Null while the launcher is up. */
    std::unique_ptr<XrLayerStack> sample_stack_;

    Mode mode_ = Mode::eLauncher;
    std::string current_sample_;
    std::string last_sample_;
    std::string pending_sample_;
    bool is_sample_requested_ = false;
    bool is_launcher_requested_ = false;
    bool is_quit_asked_ = false;
    bool is_view_placed_ = false;
    bool is_launcher_placed_ = false;
    int last_grip_count_ = 0;
    std::uint32_t reference_space_epoch_ = 0;
    /** Exit needs a second pull within a few seconds, so a stray trigger pull cannot close the app. */
    std::chrono::steady_clock::time_point exit_armed_time_{};
    bool is_exit_armed_ = false;
    std::chrono::steady_clock::time_point head_log_time_{};
};

}  // namespace vne::xr::samples
