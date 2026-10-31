/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "app/xr_sample_host.h"

#include "vertexnova/logging/logging.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <thread>

namespace vne::xr::samples {

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.xr.samples.host");

// Idle wait while the session is not running (before READY, or paused): keeps events polled without a busy spin.
constexpr auto kIdlePollInterval = std::chrono::milliseconds(10);
constexpr float kNearPlane = 0.05f;
constexpr float kSnapTurnRadians = 3.14159265f / 6.f;  // 30 degrees
constexpr float kRadiansToDegrees = 57.29578f;
constexpr auto kExitConfirmWindow = std::chrono::seconds(3);
constexpr auto kHeadLogInterval = std::chrono::seconds(5);
constexpr std::size_t kLeftHand = 0;
constexpr std::size_t kRightHand = 1;

float headingDegrees(const Pose& pose) {
    const xrmath::Vec3 forward = xrmath::rotate(xrmath::normalized(xrmath::orientationOf(pose)), {0.f, 0.f, -1.f});
    return std::atan2(-forward.x, -forward.z) * kRadiansToDegrees;
}

/** "stereo_cube" -> "Stereo Cube". */
std::string displayName(const std::string& name) {
    std::string out = name;
    bool is_word_start = true;
    for (char& c : out) {
        if (c == '_') {
            c = ' ';
        }
        c = is_word_start ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : c;
        is_word_start = c == ' ';
    }
    return out;
}

}  // namespace

XrSampleHost::XrSampleHost(const SampleRegistry& registry, SessionConfig config)
    : registry_(registry)
    , config_(std::move(config)) {}

XrSampleHost::~XrSampleHost() {
    // Layers release GPU resources on the session's device, so they go first.
    if (session_) {
        renderer_.detach();
    }
    stopSample();
    world_stack_.onDetach();
    text_renderer_.shutdown();
    session_.reset();
}

void XrSampleHost::requestSample(std::string_view name) {
    if (name.empty()) {
        is_launcher_requested_ = true;
        return;
    }
    pending_sample_ = std::string(name);
    is_sample_requested_ = true;
}

void XrSampleHost::buildOverlays() {
    std::vector<LauncherEntry> entries;
    for (const SampleInfo& info : registry_.samples()) {
        if (info.backend != config_.backend) {
            continue;
        }
        LauncherEntry entry;
        entry.id = info.name;
        entry.title = displayName(info.name);
        entry.description = info.description;
        entry.accent = info.accent_color;
        entries.push_back(std::move(entry));
    }
    LauncherEntry exit_entry;
    exit_entry.title = "Exit";
    exit_entry.description = "Close the app";
    exit_entry.accent = {0.9f, 0.3f, 0.3f};
    exit_entry.is_exit = true;
    entries.push_back(std::move(exit_entry));

    auto visuals = std::make_unique<ControllerVisualsLayer>(&text_renderer_);
    controller_visuals_ = visuals.get();
    controller_visuals_->setMenuCallback([this] { returnToLauncher(); });
    auto launcher = std::make_unique<LauncherLayer>(
        std::move(entries),
        [this](const LauncherEntry& entry) { onLauncherSelect(entry); },
        &text_renderer_);
    launcher_ = launcher.get();
    world_stack_.pushLayer(std::move(launcher));
    world_stack_.pushOverlay(std::move(visuals));
}

bool XrSampleHost::canPlace(const Frame& frame) const {
    // Before VISIBLE the runtime is not showing our frames yet and the head pose can still be the one from the
    // previous app (the Meta home): content placed with it ends up off to the side.
    const SessionState state = session_->state();
    return frame.is_head_pose_valid && (state == SessionState::eVisible || state == SessionState::eFocused);
}

void XrSampleHost::logHeadPose(const Frame& frame, const char* reason) const {
    if (!frame.is_head_pose_valid) {
        VNE_LOG_INFO << "Head pose (" << reason << "): not valid";
        return;
    }
    const Pose& head = frame.head_pose;
    VNE_LOG_INFO << "Head pose (" << reason << "): position (" << head.position[0] << ", " << head.position[1] << ", "
                 << head.position[2] << "), heading " << headingDegrees(head) << " deg";
}

void XrSampleHost::onLauncherSelect(const LauncherEntry& entry) {
    if (entry.is_exit) {
        const auto now = std::chrono::steady_clock::now();
        if (!is_exit_armed_ || now - exit_armed_time_ > kExitConfirmWindow) {
            is_exit_armed_ = true;
            exit_armed_time_ = now;
            launcher_->setStatus("Pull the trigger on Exit again to close the app");
            VNE_LOG_INFO << "Launcher: Exit selected once; waiting for confirmation";
            return;
        }
        VNE_LOG_INFO << "Launcher: Exit confirmed; ending the session";
        requestExit();
        return;
    }
    is_exit_armed_ = false;
    VNE_LOG_INFO << "Launcher: '" << entry.id << "' selected";
    requestSample(entry.id);
}

void XrSampleHost::stopSample() {
    if (sample_stack_) {
        VNE_LOG_INFO << "Stopping sample '" << current_sample_ << "'";
        sample_stack_->onDetach();
        sample_stack_.reset();
    }
    current_sample_.clear();
}

void XrSampleHost::showLauncher() {
    stopSample();
    mode_ = Mode::eLauncher;
    // Placed by applyPendingChanges() as soon as the head pose can be trusted (this frame, if it already can).
    is_launcher_placed_ = false;
    is_exit_armed_ = false;
    controller_visuals_->setRayVisible(true);
    controller_visuals_->setMenuTagVisible(false);
    VNE_LOG_INFO << "Launcher shown";
}

void XrSampleHost::startSample(const SampleInfo& info) {
    // The GPU is idle here (the session waits in beginFrame), so the old sample's resources can go.
    stopSample();
    VNE_LOG_INFO << "Starting sample '" << info.name << "'";
    sample_stack_ = std::make_unique<XrLayerStack>();
    sample_stack_->setTextRenderer(&text_renderer_);
    info.setup(*sample_stack_);
    sample_stack_->onAttach(session_.get());
    current_sample_ = info.name;
    last_sample_ = info.name;
    mode_ = Mode::eSample;
    launcher_->hide();
    launcher_->setStatus({});
    controller_visuals_->setMenuTagVisible(true);
    // A sample always starts in front of the user, at normal size: placed by applyPendingChanges().
    is_view_placed_ = false;
}

void XrSampleHost::applyPendingChanges(const Frame& frame) {
    if (is_sample_requested_) {
        is_sample_requested_ = false;
        is_launcher_requested_ = false;
        const SampleInfo* info = registry_.find(pending_sample_);
        if (info == nullptr || info->backend != config_.backend) {
            VNE_LOG_ERROR << "No runnable sample named '" << pending_sample_ << "'";
            showLauncher();
            launcher_->setStatus("No sample named " + displayName(pending_sample_));
        } else {
            startSample(*info);
        }
        pending_sample_.clear();
    } else if (is_launcher_requested_) {
        is_launcher_requested_ = false;
        if (mode_ != Mode::eLauncher || !launcher_->isVisible()) {
            showLauncher();
        }
    }

    // The runtime moved the reference space (recenter, or the floor origin settling after start): whatever was placed
    // in front of the user is now somewhere else in the room, so place it again.
    if (frame.reference_space_epoch != reference_space_epoch_) {
        reference_space_epoch_ = frame.reference_space_epoch;
        if (mode_ == Mode::eLauncher) {
            is_launcher_placed_ = false;
        } else {
            is_view_placed_ = false;
        }
        VNE_LOG_INFO << "Reference space changed; re-placing the " << (mode_ == Mode::eLauncher ? "launcher" : "scene");
    }

    // Poses arrive a few frames after the session starts: finish placing what needed one.
    if (canPlace(frame)) {
        if (mode_ == Mode::eLauncher && !is_launcher_placed_) {
            launcher_->show(frame.head_pose);
            is_launcher_placed_ = true;
            logHeadPose(frame, "launcher placed");
        }
        if (mode_ == Mode::eSample && !is_view_placed_) {
            navigator_.reset(frame.head_pose);
            is_view_placed_ = true;
            logHeadPose(frame, "scene placed");
        }
    }
}

void XrSampleHost::handleControls(const Frame& frame, const InputState& input) {
    edges_.update(input);
    if (edges_.menuPressed()) {
        if (mode_ == Mode::eSample) {
            VNE_LOG_INFO << "Menu button pressed (back to launcher)";
            returnToLauncher();
        } else if (!last_sample_.empty()) {
            VNE_LOG_INFO << "Menu button pressed (resume '" << last_sample_ << "')";
            requestSample(last_sample_);
        } else if (canPlace(frame)) {
            // Nothing to resume yet: bring the panel in front instead of ignoring the press.
            launcher_->show(frame.head_pose);
            logHeadPose(frame, "launcher re-placed by Menu");
        }
    }
    // B/Y: bring the launcher or the scene in front of you again.
    const bool is_replace_pressed = edges_.secondaryPressed(kLeftHand) || edges_.secondaryPressed(kRightHand);
    if (mode_ == Mode::eLauncher && canPlace(frame) && is_replace_pressed) {
        launcher_->show(frame.head_pose);
        logHeadPose(frame, "launcher re-placed by B/Y");
    }
    if (mode_ == Mode::eSample && frame.is_head_pose_valid) {
        if (is_replace_pressed) {
            navigator_.reset(frame.head_pose);
            is_view_placed_ = true;
            logHeadPose(frame, "scene reset by B/Y");
        }
        // Right stick flick: turn the scene 30 degrees about the user's head (a right flick looks right).
        const bool is_flicked_right = edges_.stickFlickedRight(kRightHand);
        if (is_flicked_right || edges_.stickFlickedLeft(kRightHand)) {
            const float turn = is_flicked_right ? kSnapTurnRadians : -kSnapTurnRadians;
            navigator_.snapTurn(frame.head_pose, turn);
            VNE_LOG_INFO << "Snap turn " << (turn > 0.f ? "right" : "left");
        }
    }
    if (mode_ == Mode::eSample) {
        navigator_.update(input);
        if (navigator_.gripCount() != last_grip_count_) {
            last_grip_count_ = navigator_.gripCount();
            VNE_LOG_INFO << "Scene navigation: " << last_grip_count_ << " grip(s) held, scale " << navigator_.scale();
        }
    }
}

void XrSampleHost::logHeadPoseIfDue(const Frame& frame) {
    if (const auto now = std::chrono::steady_clock::now(); now - head_log_time_ >= kHeadLogInterval) {
        head_log_time_ = now;
        logHeadPose(frame, "periodic");
    }
}

int XrSampleHost::run() {
    session_ = createSession(config_);
    if (!session_) {
        return 1;
    }
    if (session_->backendType() != BackendType::eOpenXr || !renderer_.attach(*session_)) {
        VNE_LOG_ERROR << "The sample host needs a session with a graphics device (OpenXR)";
        return 1;
    }
    if (!text_renderer_.initialize(session_->graphicsDevice())) {
        VNE_LOG_ERROR << "Text rendering is unavailable; labels will not be drawn";
    }
    buildOverlays();
    world_stack_.onAttach(session_.get());
    // The launcher panel is placed once the first head pose arrives.
    mode_ = Mode::eLauncher;
    if (!initial_sample_.empty()) {
        requestSample(initial_sample_);
    }

    Frame frame{};
    InputState input{};
    bool is_exit_requested = false;
    const float launcher_clear[3] = {0.02f, 0.025f, 0.04f};

    while (true) {
        // Ask once; the loop then runs on until the backend reports the session has ended (pollEvents() == false).
        const bool wants_quit = (frame_pump_ && !frame_pump_(*session_)) || is_quit_asked_;
        if (wants_quit && !is_exit_requested) {
            is_exit_requested = true;
            if (!session_->requestExit()) {
                break;
            }
        }
        if (!session_->pollEvents()) {
            break;
        }
        if (!session_->beginFrame(frame)) {
            if (!session_->isSessionRunning()) {
                std::this_thread::sleep_for(kIdlePollInterval);
            }
            continue;
        }
        session_->pollInput(input);

        applyPendingChanges(frame);

        handleControls(frame, input);
        logHeadPoseIfDue(frame);

        LayerParams layers{};
        // Closer than the default 0.1 m so a zoomed-in scene is not sliced open by the near plane.
        layers.near_z = kNearPlane;
        std::copy(std::begin(launcher_clear), std::end(launcher_clear), std::begin(layers.clear_color));

        FrameParams world_params{frame};
        world_params.input = input;
        LayerParams overlay_params{};
        world_stack_.onUpdate(world_params, overlay_params);

        if (mode_ == Mode::eSample && sample_stack_) {
            FrameParams scene_params{frame};
            scene_params.input = navigator_.toSceneSpace(input);
            // Layers start from the launcher background and may override it.
            sample_stack_->onUpdate(scene_params, layers);
            if (layers.request_exit) {
                requestExit();
            }
        }

        if (frame.should_render && frame.view_count > 0) {
            std::vector<RenderPart> parts;
            if (mode_ == Mode::eSample && sample_stack_) {
                parts.push_back({sample_stack_.get(), navigator_.worldFromSceneMatrix()});
            }
            parts.push_back({&world_stack_, xrmath::identity()});
            layers.submit_projection = renderer_.render(frame, parts, layers.clear_color);
        }
        session_->endFrame(frame, layers);
    }
    return 0;
}

}  // namespace vne::xr::samples
