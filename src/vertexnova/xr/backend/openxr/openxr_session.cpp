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

#include "vertexnova/xr/backend/openxr/openxr_session.h"

#include <openxr/openxr.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "vertexnova/logging/logging.h"
#include "vertexnova/rhi/device.h"
#include "vertexnova/xr/backend/openxr/openxr_platform_prelude.h"
#include "vertexnova/xr/core/session_state.h"
#include "vertexnova/xr/backend/openxr/math/openxr_math.h"
#include "vertexnova/xr/backend/openxr/openxr_instance_lifetime.h"
#include "vertexnova/xr/backend/openxr/openxr_util.h"
#include "vertexnova/xr/backend/openxr/platform/openxr_android.h"
#include "vertexnova/xr/backend/openxr/platform/openxr_debug.h"

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.xr.openxr.session");
}

namespace vne::xr {

namespace {

void storePose(const XrPosef& src, Pose& dst) {
    dst.position[0] = src.position.x;
    dst.position[1] = src.position.y;
    dst.position[2] = src.position.z;
    dst.orientation[0] = src.orientation.x;
    dst.orientation[1] = src.orientation.y;
    dst.orientation[2] = src.orientation.z;
    dst.orientation[3] = src.orientation.w;
}

void storeFov(const XrFovf& src, FieldOfView& dst) {
    dst.angle_left = src.angleLeft;
    dst.angle_right = src.angleRight;
    dst.angle_up = src.angleUp;
    dst.angle_down = src.angleDown;
}

XrPosef identityPose() {
    XrPosef pose{};
    pose.orientation.w = 1.f;
    return pose;
}

[[nodiscard]] const char* sessionStateName(XrSessionState xr_state) {
    switch (xr_state) {
        case XR_SESSION_STATE_IDLE:
            return "IDLE";
        case XR_SESSION_STATE_READY:
            return "READY";
        case XR_SESSION_STATE_SYNCHRONIZED:
            return "SYNCHRONIZED";
        case XR_SESSION_STATE_VISIBLE:
            return "VISIBLE";
        case XR_SESSION_STATE_FOCUSED:
            return "FOCUSED";
        case XR_SESSION_STATE_STOPPING:
            return "STOPPING";
        case XR_SESSION_STATE_LOSS_PENDING:
            return "LOSS_PENDING";
        case XR_SESSION_STATE_EXITING:
            return "EXITING";
        default:
            return "UNKNOWN";
    }
}

[[nodiscard]] SessionState toSessionState(XrSessionState xr_state) {
    switch (xr_state) {
        case XR_SESSION_STATE_IDLE:
            return SessionState::eIdle;
        case XR_SESSION_STATE_READY:
            return SessionState::eReady;
        case XR_SESSION_STATE_SYNCHRONIZED:
            return SessionState::eSynchronized;
        case XR_SESSION_STATE_VISIBLE:
            return SessionState::eVisible;
        case XR_SESSION_STATE_FOCUSED:
            return SessionState::eFocused;
        case XR_SESSION_STATE_STOPPING:
            return SessionState::eStopping;
        case XR_SESSION_STATE_LOSS_PENDING:
            return SessionState::eLossPending;
        case XR_SESSION_STATE_EXITING:
            return SessionState::eExiting;
        default:
            return SessionState::eUnknown;
    }
}

constexpr auto kFrameRetryInterval = std::chrono::milliseconds(5);
constexpr auto kHealthLogInterval = std::chrono::seconds(5);
constexpr int kShutdownDrainAttempts = 100;
constexpr auto kShutdownDrainInterval = std::chrono::milliseconds(10);

}  // namespace

OpenXrSession::OpenXrSession(SessionConfig config)
    : config_(std::move(config)) {
#if defined(__ANDROID__)
    if (config_.platform.application_vm != nullptr && config_.platform.application_context != nullptr) {
        if (isError(
                initializeOpenXrLoaderAndroid(config_.platform.application_vm, config_.platform.application_context))) {
            state_ = SessionState::eExiting;
            running_ = false;
        }
    }
#endif
    if (running_ && !initOpenXr()) {
        state_ = SessionState::eExiting;
        running_ = false;
    }
}

OpenXrSession::~OpenXrSession() {
    shutdownOpenXr();
}

BackendType OpenXrSession::backendType() const {
    return BackendType::eOpenXr;
}

SessionState OpenXrSession::state() const {
    return state_;
}

bool OpenXrSession::isSessionRunning() const {
    return session_running_;
}

bool OpenXrSession::initOpenXr() {
    const auto available = OpenXrUtil::enumerateInstanceExtensions();
    std::vector<std::string> requested;
#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
    if (OpenXrUtil::isExtensionAvailable(XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME, available)) {
        requested.emplace_back(XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME);
        use_vulkan_enable2_ = true;
    } else if (OpenXrUtil::isExtensionAvailable(XR_KHR_VULKAN_ENABLE_EXTENSION_NAME, available)) {
        requested.emplace_back(XR_KHR_VULKAN_ENABLE_EXTENSION_NAME);
    }
#endif
#if defined(__ANDROID__)
    requested.emplace_back(XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME);
#endif
#ifndef XR_EXT_HAND_INTERACTION_EXTENSION_NAME
#define XR_EXT_HAND_INTERACTION_EXTENSION_NAME "XR_EXT_hand_interaction"
#endif
    if (OpenXrUtil::isExtensionAvailable(XR_EXT_HAND_INTERACTION_EXTENSION_NAME, available)) {
        requested.emplace_back(XR_EXT_HAND_INTERACTION_EXTENSION_NAME);
    }
    if (config_.use_compositor_supersampling
        && OpenXrUtil::isExtensionAvailable(XR_FB_COMPOSITION_LAYER_SETTINGS_EXTENSION_NAME, available)) {
        requested.emplace_back(XR_FB_COMPOSITION_LAYER_SETTINGS_EXTENSION_NAME);
        has_layer_settings_ = true;
    }
    if (OpenXrUtil::isExtensionAvailable(XR_EXT_LOCAL_FLOOR_EXTENSION_NAME, available)) {
        requested.emplace_back(XR_EXT_LOCAL_FLOOR_EXTENSION_NAME);
    }
    requested.emplace_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);

    auto enabled = OpenXrUtil::filterEnabledExtensions(requested, available);
    std::vector<std::string> enabled_storage;
    enabled_storage.reserve(enabled.size());
    for (const char* ext : enabled) {
        enabled_storage.emplace_back(ext);
    }
    std::vector<const char*> enabled_ptrs;
    enabled_ptrs.reserve(enabled_storage.size());
    for (const auto& ext : enabled_storage) {
        enabled_ptrs.push_back(ext.c_str());
    }

    XrInstanceCreateInfo create_info = OpenXrUtil::makeXrStruct<XrInstanceCreateInfo>(XR_TYPE_INSTANCE_CREATE_INFO);
    create_info.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    OpenXrUtil::copyXrFixedString(create_info.applicationInfo.applicationName,
                                  XR_MAX_APPLICATION_NAME_SIZE,
                                  config_.application_name.c_str());
    create_info.enabledExtensionCount = static_cast<std::uint32_t>(enabled_ptrs.size());
    create_info.enabledExtensionNames = enabled_ptrs.data();

#if defined(__ANDROID__)
    // XR_KHR_android_create_instance: runtimes on Android need the VM and activity at instance creation.
    XrInstanceCreateInfoAndroidKHR android_info =
        OpenXrUtil::makeXrStruct<XrInstanceCreateInfoAndroidKHR>(XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR);
    const bool is_android_create_enabled = std::any_of(enabled_ptrs.begin(), enabled_ptrs.end(), [](const char* name) {
        return std::strcmp(name, XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME) == 0;
    });
    if (is_android_create_enabled && config_.platform.application_vm != nullptr
        && config_.platform.application_context != nullptr) {
        android_info.applicationVM = config_.platform.application_vm;
        android_info.applicationActivity = config_.platform.application_context;
        create_info.next = &android_info;
    } else {
        VNE_LOG_ERROR << "OpenXR on Android needs XR_KHR_android_create_instance plus the application VM and activity";
        return false;
    }
#endif

    OPENXR_CHECK_BOOL(xrCreateInstance(&create_info, &instance_));
    OpenXrInstanceLifetime::registerInstanceUser(instance_);

    // Kept for the whole session so the runtime's validation messages during frames reach the log.
    if (!debug_messenger_.create(instance_, &xrGetInstanceProcAddr)) {
        VNE_LOG_WARN << "OpenXR debug messenger not available";
    }

    XrInstanceProperties instance_properties =
        OpenXrUtil::makeXrStruct<XrInstanceProperties>(XR_TYPE_INSTANCE_PROPERTIES);
    if (XR_SUCCEEDED(xrGetInstanceProperties(instance_, &instance_properties))) {
        const std::uint64_t version = instance_properties.runtimeVersion;
        VNE_LOG_INFO << "OpenXR runtime: " << instance_properties.runtimeName << " " << ((version >> 48) & 0xffffU)
                     << "." << ((version >> 32) & 0xffffU) << "." << (version & 0xffffffffU);
    }

    XrSystemGetInfo system_info = OpenXrUtil::makeXrStruct<XrSystemGetInfo>(XR_TYPE_SYSTEM_GET_INFO);
    system_info.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    OPENXR_CHECK_BOOL(xrGetSystem(instance_, &system_info, &system_id_));

    std::uint32_t view_config_count = 0;
    OPENXR_CHECK_BOOL(xrEnumerateViewConfigurations(instance_, system_id_, 0, &view_config_count, nullptr));
    std::vector<XrViewConfigurationType> view_configs_types(view_config_count);
    OPENXR_CHECK_BOOL(xrEnumerateViewConfigurations(instance_,
                                                    system_id_,
                                                    view_config_count,
                                                    &view_config_count,
                                                    view_configs_types.data()));

    view_configuration_ = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    if (std::find(view_configs_types.begin(), view_configs_types.end(), view_configuration_)
        == view_configs_types.end()) {
        if (!view_configs_types.empty()) {
            view_configuration_ = view_configs_types.front();
        }
    }

    std::uint32_t view_count = 0;
    OPENXR_CHECK_BOOL(
        xrEnumerateViewConfigurationViews(instance_, system_id_, view_configuration_, 0, &view_count, nullptr));
    view_configs_.resize(view_count,
                         OpenXrUtil::makeXrStruct<XrViewConfigurationView>(XR_TYPE_VIEW_CONFIGURATION_VIEW));
    OPENXR_CHECK_BOOL(xrEnumerateViewConfigurationViews(instance_,
                                                        system_id_,
                                                        view_configuration_,
                                                        view_count,
                                                        &view_count,
                                                        view_configs_.data()));

    std::uint32_t blend_count = 0;
    OPENXR_CHECK_BOOL(
        xrEnumerateEnvironmentBlendModes(instance_, system_id_, view_configuration_, 0, &blend_count, nullptr));
    std::vector<XrEnvironmentBlendMode> blend_modes(blend_count);
    OPENXR_CHECK_BOOL(xrEnumerateEnvironmentBlendModes(instance_,
                                                       system_id_,
                                                       view_configuration_,
                                                       blend_count,
                                                       &blend_count,
                                                       blend_modes.data()));
    blend_mode_ = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    if (std::find(blend_modes.begin(), blend_modes.end(), blend_mode_) == blend_modes.end()) {
        if (!blend_modes.empty()) {
            blend_mode_ = blend_modes.front();
        }
    }

    OpenXrVulkanGraphics::Attribs gfx_attribs{};
    gfx_attribs.instance = instance_;
    gfx_attribs.system_id = system_id_;
    gfx_attribs.get_instance_proc_addr = &xrGetInstanceProcAddr;
    gfx_attribs.use_vulkan_enable2 = use_vulkan_enable2_;
    VNE_XR_CHECK_BOOL(graphics_.initialize(gfx_attribs));
    VNE_XR_CHECK_BOOL(graphics_.createDevice());

    XrSessionCreateInfo session_info = OpenXrUtil::makeXrStruct<XrSessionCreateInfo>(XR_TYPE_SESSION_CREATE_INFO);
    session_info.systemId = system_id_;
#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
    XrGraphicsBindingVulkanKHR vulkan_binding = graphics_.getGraphicsBinding();
    session_info.next = &vulkan_binding;
#endif
    OPENXR_CHECK_BOOL(xrCreateSession(instance_, &session_info, &session_));

    reference_space_type_ = chooseReferenceSpaceType();
    XrReferenceSpaceCreateInfo space_info =
        OpenXrUtil::makeXrStruct<XrReferenceSpaceCreateInfo>(XR_TYPE_REFERENCE_SPACE_CREATE_INFO);
    space_info.referenceSpaceType = reference_space_type_;
    space_info.poseInReferenceSpace = identityPose();
    OPENXR_CHECK_BOOL(xrCreateReferenceSpace(session_, &space_info, &reference_space_));

    XrReferenceSpaceCreateInfo view_space_info =
        OpenXrUtil::makeXrStruct<XrReferenceSpaceCreateInfo>(XR_TYPE_REFERENCE_SPACE_CREATE_INFO);
    view_space_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    view_space_info.poseInReferenceSpace = identityPose();
    OPENXR_CHECK_BOOL(xrCreateReferenceSpace(session_, &view_space_info, &view_space_));

    if (isError(input_.create(instance_, session_, reference_space_))) {
        VNE_LOG_WARN << "OpenXR input setup failed; continuing without actions";
    }

    std::uint32_t format_count = 0;
    OPENXR_CHECK_BOOL(xrEnumerateSwapchainFormats(session_, 0, &format_count, nullptr));
    std::vector<int64_t> formats(format_count);
    OPENXR_CHECK_BOOL(xrEnumerateSwapchainFormats(session_, format_count, &format_count, formats.data()));
    if (!formats.empty()) {
#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
        // Accept only formats with an exact RHI mapping; keep runtime preference order.
        constexpr int64_t kSupportedColorFormats[] = {
            static_cast<int64_t>(VK_FORMAT_R8G8B8A8_SRGB),
            static_cast<int64_t>(VK_FORMAT_B8G8R8A8_SRGB),
            static_cast<int64_t>(VK_FORMAT_R8G8B8A8_UNORM),
            static_cast<int64_t>(VK_FORMAT_B8G8R8A8_UNORM),
            static_cast<int64_t>(VK_FORMAT_R16G16B16A16_SFLOAT),
            static_cast<int64_t>(VK_FORMAT_R32G32B32A32_SFLOAT),
        };
        color_format_ = 0;
        for (const auto format : formats) {
            bool supported = false;
            for (const auto candidate : kSupportedColorFormats) {
                if (format == candidate) {
                    supported = true;
                    break;
                }
            }
            if (supported) {
                color_format_ = format;
                break;
            }
        }
        if (color_format_ == 0) {
            VNE_LOG_ERROR << "OpenXR: no supported color swapchain format among " << formats.size()
                          << " runtime formats";
            return false;
        }
#else
        color_format_ = formats.front();
#endif
    }

    state_ = SessionState::eIdle;
    VNE_LOG_INFO << "OpenXR session initialized (MSAA request=" << config_.color_sample_count
                 << ", resolution scale=" << config_.resolution_scale
                 << ", compositor super sampling=" << (has_layer_settings_ ? "on" : "off")
                 << ", gpu sync every frame=" << config_.is_gpu_sync_every_frame << ", enable2=" << use_vulkan_enable2_
                 << ", reference space=" << static_cast<int>(reference_space_type_)
                 << ", color format=" << color_format_ << ", views=" << view_configs_.size() << ")";
    return true;
}

void OpenXrSession::shutdownOpenXr() {
    // A begun session must be taken through STOPPING before it is destroyed. Ask for the exit and pump events for
    // a bounded time; destroying the session below is the fallback when the runtime does not answer.
    if (session_begun_ && session_ != XR_NULL_HANDLE && instance_ != XR_NULL_HANDLE) {
        static_cast<void>(requestExit());
        for (int attempt = 0; attempt < kShutdownDrainAttempts && session_begun_; ++attempt) {
            processPendingEvents();
            if (!session_begun_ || !running_) {
                // Ended, or the runtime/instance is going away: there is nothing further to wait for.
                break;
            }
            std::this_thread::sleep_for(kShutdownDrainInterval);
        }
    }

    flushGpu();
    input_.destroy();
    releaseSwapchains();

    if (view_space_ != XR_NULL_HANDLE) {
        xrDestroySpace(view_space_);
        view_space_ = XR_NULL_HANDLE;
    }
    if (reference_space_ != XR_NULL_HANDLE) {
        xrDestroySpace(reference_space_);
        reference_space_ = XR_NULL_HANDLE;
    }
    if (session_ != XR_NULL_HANDLE) {
        xrDestroySession(session_);
        session_ = XR_NULL_HANDLE;
    }

    graphics_.shutdown();

    if (instance_ != XR_NULL_HANDLE) {
        debug_messenger_.destroy(instance_);
        OpenXrInstanceLifetime::unregisterInstanceUser(instance_);
        xrDestroyInstance(instance_);
        instance_ = XR_NULL_HANDLE;
    }
    session_running_ = false;
}

void OpenXrSession::releaseSwapchains() {
    if (are_images_acquired_) {
        static_cast<void>(swapchain_bridge_.releaseForFrame());
        are_images_acquired_ = false;
    }
    swapchain_bridge_.destroy();
    swapchains_ready_ = false;
}

XrReferenceSpaceType OpenXrSession::chooseReferenceSpaceType() const {
    std::uint32_t count = 0;
    std::vector<XrReferenceSpaceType> types;
    if (XR_SUCCEEDED(xrEnumerateReferenceSpaces(session_, 0, &count, nullptr)) && count > 0) {
        types.resize(count);
        if (XR_FAILED(xrEnumerateReferenceSpaces(session_, count, &count, types.data()))) {
            types.clear();
        }
    }
    const auto is_supported = [&types](XrReferenceSpaceType type) {
        return std::find(types.begin(), types.end(), type) != types.end();
    };
    // Floor-relative content (a table at 0.8 m) needs an origin on the floor: LOCAL_FLOOR when the runtime
    // offers it, else STAGE; LOCAL puts the origin at the head and is the last resort.
    if (is_supported(XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT)) {
        return XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT;
    }
    if (is_supported(XR_REFERENCE_SPACE_TYPE_STAGE)) {
        return XR_REFERENCE_SPACE_TYPE_STAGE;
    }
    return XR_REFERENCE_SPACE_TYPE_LOCAL;
}

void OpenXrSession::handleSessionStateChanged(XrSessionState xr_state) {
    const SessionState next = toSessionState(xr_state);
    if (next == SessionState::eUnknown) {
        return;
    }
    VNE_LOG_INFO << "OpenXR session state: " << sessionStateName(xr_state);
    const SessionTransition transition = evaluateSessionTransition(next, session_begun_);
    state_ = transition.state;
    session_running_ = transition.is_frame_loop_active;

    if (transition.should_begin_session) {
        XrSessionBeginInfo begin = OpenXrUtil::makeXrStruct<XrSessionBeginInfo>(XR_TYPE_SESSION_BEGIN_INFO);
        begin.primaryViewConfigurationType = view_configuration_;
        if (XR_SUCCEEDED(xrBeginSession(session_, &begin))) {
            session_begun_ = true;
            session_running_ = true;
        } else {
            VNE_LOG_ERROR << "xrBeginSession failed";
        }
    }
    if (transition.should_release_swapchains) {
        flushGpu();
        releaseSwapchains();
    }
    if (transition.should_end_session) {
        if (XR_FAILED(xrEndSession(session_))) {
            VNE_LOG_WARN << "xrEndSession failed";
        }
        session_begun_ = false;
        // The next session starts with a new tracking state: poses from this one must not be reused.
        has_located_views_ = false;
        last_located_view_count_ = 0;
    }
    if (transition.should_leave_loop) {
        running_ = false;
    }
}

bool OpenXrSession::ensureSwapchains() {
    if (swapchains_ready_) {
        return true;
    }
    if (view_configs_.empty() || color_format_ == 0) {
        return false;
    }
    if (!swapchain_bridge_
             .create(session_, scaledViewConfigs(), color_format_, config_.color_sample_count, graphics_.getDevice())) {
        return false;
    }
    swapchains_ready_ = true;
    return true;
}

std::shared_ptr<vne::rhi::IDevice> OpenXrSession::graphicsDevice() const {
    return graphics_.getDevice();
}

void OpenXrSession::flushGpu() {
    if (auto device = graphics_.getDevice()) {
        // IDevice::waitIdle() is implemented in vnerhi. Calling vk::Device::waitIdle()
        // from this DLL references vk::detail::defaultDispatchLoaderDynamic, which is
        // defined only in the RHI library.
        device->waitIdle();
    }
}

void OpenXrSession::processPendingEvents() {
    XrEventDataBuffer event = OpenXrUtil::makeXrStruct<XrEventDataBuffer>(XR_TYPE_EVENT_DATA_BUFFER);
    while (xrPollEvent(instance_, &event) == XR_SUCCESS) {
        switch (event.type) {
            case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
                const auto* state_event = reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
                if (state_event->session == session_) {
                    handleSessionStateChanged(state_event->state);
                }
                break;
            }
            case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
                VNE_LOG_WARN << "OpenXR instance loss pending; leaving the loop";
                state_ = SessionState::eLossPending;
                session_running_ = false;
                running_ = false;
                break;
            case XR_TYPE_EVENT_DATA_EVENTS_LOST:
                VNE_LOG_WARN << "OpenXR event queue overflowed; "
                             << reinterpret_cast<XrEventDataEventsLost*>(&event)->lostEventCount << " events lost";
                break;
            case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING: {
                const auto* change = reinterpret_cast<XrEventDataReferenceSpaceChangePending*>(&event);
                if (change->session == session_ && change->referenceSpaceType == reference_space_type_) {
                    // Content placed in the old coordinates would now sit somewhere else in the room.
                    has_space_change_pending_ = true;
                    space_change_time_ = change->changeTime;
                    VNE_LOG_INFO << "OpenXR reference space changes at " << change->changeTime
                                 << " (recenter or floor origin update)";
                }
                break;
            }
            case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
                input_.logCurrentProfiles();
                break;
            default:
                break;
        }
        event = OpenXrUtil::makeXrStruct<XrEventDataBuffer>(XR_TYPE_EVENT_DATA_BUFFER);
    }
}

bool OpenXrSession::pollEvents() {
    if (!running_ || instance_ == XR_NULL_HANDLE) {
        return false;
    }
    processPendingEvents();
    return running_;
}

bool OpenXrSession::requestExit() {
    if (session_ == XR_NULL_HANDLE || !session_begun_) {
        // Nothing is running, so there is no STOPPING to wait for.
        running_ = false;
        return true;
    }
    if (!is_exit_requested_) {
        is_exit_requested_ = true;
        if (XR_FAILED(xrRequestExitSession(session_))) {
            VNE_LOG_WARN << "xrRequestExitSession failed; leaving the loop without waiting for STOPPING";
            running_ = false;
        }
    }
    return true;
}

bool OpenXrSession::locateViews(XrTime display_time, ViewArray& out_views, std::uint32_t& out_count) {
    XrViewLocateInfo locate_info = OpenXrUtil::makeXrStruct<XrViewLocateInfo>(XR_TYPE_VIEW_LOCATE_INFO);
    locate_info.viewConfigurationType = view_configuration_;
    locate_info.displayTime = display_time;
    locate_info.space = reference_space_;

    XrViewState view_state = OpenXrUtil::makeXrStruct<XrViewState>(XR_TYPE_VIEW_STATE);
    out_views.fill(OpenXrUtil::makeXrStruct<XrView>(XR_TYPE_VIEW));
    out_count = kViewCapacity;
    const bool is_located =
        XR_SUCCEEDED(xrLocateViews(session_, &locate_info, &view_state, out_count, &out_count, out_views.data()));
    const bool is_orientation_valid =
        is_located && (view_state.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) != 0;
    const bool is_position_valid = is_located && (view_state.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) != 0;

    // A tracking hiccup (often position only) must not blank the display: draw with the last located pose instead.
    if (is_orientation_valid && out_count > 0) {
        if (!is_position_valid && has_located_views_) {
            for (std::uint32_t i = 0; i < out_count; ++i) {
                out_views[i].pose.position = last_located_views_[i].pose.position;
            }
            ++health_.stale_pose_frames;
        }
        if (is_position_valid || has_located_views_) {
            last_located_views_ = out_views;
            last_located_view_count_ = out_count;
            has_located_views_ = true;
            return true;
        }
    } else if (has_located_views_) {
        out_views = last_located_views_;
        out_count = last_located_view_count_;
        ++health_.stale_pose_frames;
        return true;
    }
    return false;
}

bool OpenXrSession::locateHead(XrTime display_time, Pose& out_pose) const {
    if (view_space_ == XR_NULL_HANDLE) {
        return false;
    }
    XrSpaceLocation head = OpenXrUtil::makeXrStruct<XrSpaceLocation>(XR_TYPE_SPACE_LOCATION);
    if (XR_FAILED(xrLocateSpace(view_space_, reference_space_, display_time, &head))) {
        return false;
    }
    constexpr XrSpaceLocationFlags kHeadValid =
        XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
    if ((head.locationFlags & kHeadValid) != kHeadValid) {
        return false;
    }
    storePose(head.pose, out_pose);
    return true;
}

bool OpenXrSession::beginFrame(Frame& out_frame) {
    if (!running_ || session_ == XR_NULL_HANDLE || !session_running_) {
        return false;
    }

    if (swapchain_bridge_.needsRecreate()) {
        VNE_LOG_WARN << "OpenXR swapchains are stuck; recreating them";
        flushGpu();
        releaseSwapchains();
        ++health_.swapchain_recreations;
    }

    XrFrameWaitInfo wait_info = OpenXrUtil::makeXrStruct<XrFrameWaitInfo>(XR_TYPE_FRAME_WAIT_INFO);
    XrFrameState frame_state = OpenXrUtil::makeXrStruct<XrFrameState>(XR_TYPE_FRAME_STATE);
    const XrResult wait_result = xrWaitFrame(session_, &wait_info, &frame_state);
    if (XR_FAILED(wait_result)) {
        ++health_.wait_frame_failures;
        if (wait_result == XR_ERROR_SESSION_LOST) {
            VNE_LOG_ERROR << "OpenXR session lost; leaving the loop";
            state_ = SessionState::eLossPending;
            session_running_ = false;
            running_ = false;
        } else {
            // Transient around state changes: do not spin while the runtime settles.
            std::this_thread::sleep_for(kFrameRetryInterval);
        }
        return false;
    }

    // The previous frame's GPU work has had a whole frame period to finish, so this wait is normally free. It makes
    // the next two steps safe: resources queued for destruction are reclaimed and the descriptor pools rotate.
    flushGpu();
    if (auto device = graphics_.getDevice()) {
        device->beginFrame();
    }

    XrFrameBeginInfo begin_info = OpenXrUtil::makeXrStruct<XrFrameBeginInfo>(XR_TYPE_FRAME_BEGIN_INFO);
    if (XR_FAILED(xrBeginFrame(session_, &begin_info))) {
        ++health_.wait_frame_failures;
        std::this_thread::sleep_for(kFrameRetryInterval);
        return false;
    }

    predicted_display_time_ = frame_state.predictedDisplayTime;
    if (has_space_change_pending_ && frame_state.predictedDisplayTime >= space_change_time_) {
        has_space_change_pending_ = false;
        ++reference_space_epoch_;
        // Poses remembered in the old coordinates are meaningless now.
        has_located_views_ = false;
        last_located_view_count_ = 0;
        VNE_LOG_INFO << "OpenXR reference space changed (epoch " << reference_space_epoch_ << ")";
    }
    out_frame = {};
    out_frame.reference_space_epoch = reference_space_epoch_;
    out_frame.frame_index = frame_index_++;
    out_frame.predicted_display_time = static_cast<double>(frame_state.predictedDisplayTime);
    out_frame.should_render = frame_state.shouldRender == XR_TRUE;
    ++health_.frames;
    logHealth();

    if (!out_frame.should_render) {
        return true;
    }

    ViewArray xr_views{};
    std::uint32_t view_count = 0;
    if (!locateViews(frame_state.predictedDisplayTime, xr_views, view_count)) {
        // Nothing has ever been tracked: the frame is still ended, with no layers.
        ++health_.skipped_no_pose;
        out_frame.should_render = false;
        return true;
    }
    out_frame.is_head_pose_valid = locateHead(frame_state.predictedDisplayTime, out_frame.head_pose);

    out_frame.view_count = view_count;
    out_frame.surfaces.view_count = view_count;
    swapchain_bridge_.setCachedViews(xr_views.data(), view_count);
    for (std::uint32_t i = 0; i < view_count && i < Frame::kMaxViews; ++i) {
        out_frame.views[i].view_index = i;
        OpenXrMath::populateViewMatrices(xr_views[i], near_z_, far_z_, out_frame.views[i]);
        storePose(xr_views[i].pose, out_frame.views[i].pose);
        storeFov(xr_views[i].fov, out_frame.views[i].fov);
        out_frame.surfaces.views[i].view_index = i;
    }

    if (!ensureSwapchains()) {
        ++health_.acquire_failures;
        out_frame.should_render = false;
        out_frame.view_count = 0;
        out_frame.surfaces.view_count = 0;
        return true;
    }
    are_images_acquired_ = true;
    if (!swapchain_bridge_.acquireForFrame(out_frame)) {
        // Give back what was acquired and submit no layer for this frame; a stuck image triggers a rebuild.
        ++health_.acquire_failures;
        static_cast<void>(swapchain_bridge_.releaseForFrame());
        are_images_acquired_ = false;
        out_frame.should_render = false;
        out_frame.view_count = 0;
        out_frame.surfaces.view_count = 0;
    }
    return true;
}

void OpenXrSession::pollInput(InputState& out_input) {
    if (session_ == XR_NULL_HANDLE) {
        out_input = {};
        return;
    }
    if (isError(input_.poll(predicted_display_time_, out_input))) {
        out_input = {};
    }
}

void OpenXrSession::endFrame(const Frame& frame, const LayerParams& layers) {
    near_z_ = layers.near_z;
    far_z_ = layers.far_z;

    if (session_ == XR_NULL_HANDLE) {
        return;
    }

    // The app's work was submitted to the queue bound in the graphics binding before this call, and the runtime
    // orders its use of the image after that submission. The CPU does not wait here; beginFrame() waits for the GPU.
    // `is_gpu_sync_every_frame` (default on) waits anyway: the headset showed random pixel blocks without it.
    if (config_.is_gpu_sync_every_frame) {
        flushGpu();
    }
    // Release exactly what beginFrame() acquired: releasing without an acquire is a call-order error.
    const bool has_images = are_images_acquired_;
    if (has_images) {
        if (!swapchain_bridge_.releaseForFrame()) {
            VNE_LOG_WARN << "Some swapchain images were not released; the swapchains will be recreated";
        }
        are_images_acquired_ = false;
    }

    // These must outlive xrEndFrame, so they live at function scope rather than inside the branch below.
    XrCompositionLayerProjection layer =
        OpenXrUtil::makeXrStruct<XrCompositionLayerProjection>(XR_TYPE_COMPOSITION_LAYER_PROJECTION);
    std::array<XrCompositionLayerProjectionView, 2> projection_views{};
    const XrCompositionLayerBaseHeader* layer_headers[1] = {nullptr};
    XrCompositionLayerSettingsFB layer_settings =
        OpenXrUtil::makeXrStruct<XrCompositionLayerSettingsFB>(XR_TYPE_COMPOSITION_LAYER_SETTINGS_FB);
    layer_settings.layerFlags = XR_COMPOSITION_LAYER_SETTINGS_NORMAL_SUPER_SAMPLING_BIT_FB;

    XrFrameEndInfo end_info = OpenXrUtil::makeXrStruct<XrFrameEndInfo>(XR_TYPE_FRAME_END_INFO);
    end_info.displayTime = predicted_display_time_;
    end_info.environmentBlendMode = blend_mode_;
    // Only an image the app fully drew may reach the compositor: anything else would show leftover GPU memory.
    const bool is_drawn = has_images && frame.should_render && frame.view_count > 0 && layers.submit_projection;
    if (has_images && frame.should_render && !is_drawn) {
        ++health_.unrendered_frames;
    }
    if (is_drawn) {
        composition_layer_.buildProjectionLayer(reference_space_,
                                                swapchain_bridge_,
                                                frame.view_count,
                                                layer,
                                                projection_views);
        if (has_layer_settings_) {
            // The compositor filters the eye image when resampling it for the lenses: less shimmer on edges.
            layer.next = &layer_settings;
        }
        layer_headers[0] = reinterpret_cast<const XrCompositionLayerBaseHeader*>(&layer);
        end_info.layerCount = 1;
        end_info.layers = layer_headers;
    }

    const XrResult end_result = xrEndFrame(session_, &end_info);
    if (XR_FAILED(end_result)) {
        ++health_.end_frame_failures;
        VNE_LOG_WARN << "xrEndFrame failed (XrResult=" << end_result << ")";
    }
}

std::vector<XrViewConfigurationView> OpenXrSession::scaledViewConfigs() const {
    std::vector<XrViewConfigurationView> scaled = view_configs_;
    const float scale = std::clamp(config_.resolution_scale, 0.5f, 2.f);
    for (auto& view : scaled) {
        const auto resize = [scale](std::uint32_t recommended, std::uint32_t maximum) {
            // Even sizes keep the two eyes and any later downsampling aligned.
            auto size = static_cast<std::uint32_t>(static_cast<float>(recommended) * scale + 0.5f) & ~1u;
            if (maximum > 0) {
                size = std::min(size, maximum);
            }
            return std::max(size, 2u);
        };
        view.recommendedImageRectWidth = resize(view.recommendedImageRectWidth, view.maxImageRectWidth);
        view.recommendedImageRectHeight = resize(view.recommendedImageRectHeight, view.maxImageRectHeight);
    }
    return scaled;
}

void OpenXrSession::logHealth() {
    const auto now = std::chrono::steady_clock::now();
    if (now - health_log_time_ < kHealthLogInterval) {
        return;
    }
    const double seconds = std::chrono::duration<double>(now - health_log_time_).count();
    const FrameHealth& h = health_;
    const bool has_problem = h.skipped_no_pose + h.stale_pose_frames + h.unrendered_frames + h.acquire_failures
                                 + h.swapchain_recreations + h.wait_frame_failures + h.end_frame_failures
                             > 0;
    if (has_problem) {
        VNE_LOG_WARN << "XR frame health: " << h.frames << " frames in " << seconds
                     << "s; no-pose=" << h.skipped_no_pose << " stale-pose=" << h.stale_pose_frames
                     << " not-drawn=" << h.unrendered_frames << " acquire-fail=" << h.acquire_failures
                     << " swapchain-rebuilds=" << h.swapchain_recreations << " wait-fail=" << h.wait_frame_failures
                     << " end-fail=" << h.end_frame_failures;
    } else {
        VNE_LOG_INFO << "XR frame health: " << h.frames << " frames in " << seconds << "s, no problems";
    }
    health_ = {};
    health_log_time_ = now;
}

}  // namespace vne::xr
