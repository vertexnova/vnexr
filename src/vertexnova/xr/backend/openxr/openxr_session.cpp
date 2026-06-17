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

#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
#include <vulkan/vulkan.hpp>
#endif

#include <algorithm>
#include <array>
#include <cstring>
#include <string>
#include <vector>

#include "vertexnova/logging/logging.h"
#include "vertexnova/rhi/backend/vulkan/vulkan_device.h"
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
    if (running_ && isError(initOpenXr())) {
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

    XrInstanceCreateInfo create_info{XR_TYPE_INSTANCE_CREATE_INFO};
    create_info.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    std::strncpy(create_info.applicationInfo.applicationName,
                 config_.application_name.c_str(),
                 XR_MAX_APPLICATION_NAME_SIZE - 1);
    create_info.enabledExtensionCount = static_cast<std::uint32_t>(enabled_ptrs.size());
    create_info.enabledExtensionNames = enabled_ptrs.data();

    OPENXR_CHECK_BOOL(xrCreateInstance(&create_info, &instance_));
    OpenXrInstanceLifetime::registerInstanceUser(instance_);

    OpenXrDebugMessenger debug;
    debug.create(instance_, &xrGetInstanceProcAddr);

    XrSystemGetInfo system_info{XR_TYPE_SYSTEM_GET_INFO};
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
    view_configs_.resize(view_count, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
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

    XrSessionCreateInfo session_info{XR_TYPE_SESSION_CREATE_INFO};
    session_info.systemId = system_id_;
#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
    XrGraphicsBindingVulkanKHR vulkan_binding = graphics_.getGraphicsBinding();
    session_info.next = &vulkan_binding;
#endif
    OPENXR_CHECK_BOOL(xrCreateSession(instance_, &session_info, &session_));

    XrReferenceSpaceCreateInfo space_info{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    space_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    space_info.poseInReferenceSpace = identityPose();
    OPENXR_CHECK_BOOL(xrCreateReferenceSpace(session_, &space_info, &reference_space_));

    if (isError(input_.create(instance_, session_))) {
        VNE_LOG_WARN << "OpenXR input setup failed; continuing without actions";
    }

    std::uint32_t format_count = 0;
    OPENXR_CHECK_BOOL(xrEnumerateSwapchainFormats(session_, 0, &format_count, nullptr));
    std::vector<int64_t> formats(format_count);
    OPENXR_CHECK_BOOL(xrEnumerateSwapchainFormats(session_, format_count, &format_count, formats.data()));
    if (!formats.empty()) {
        color_format_ = formats.front();
#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
        for (const auto format : formats) {
            if (format == static_cast<int64_t>(VK_FORMAT_D32_SFLOAT)
                || format == static_cast<int64_t>(VK_FORMAT_D16_UNORM)) {
                depth_format_ = format;
                break;
            }
        }
#endif
    }

    debug.destroy(instance_);
    state_ = SessionState::eIdle;
    VNE_LOG_INFO << "OpenXR session initialized (enable2=" << use_vulkan_enable2_ << ")";
    return true;
}

void OpenXrSession::shutdownOpenXr() {
    input_.destroy();
    swapchain_bridge_.destroy();
    swapchains_ready_ = false;

    if (session_begun_ && session_ != XR_NULL_HANDLE) {
        xrEndSession(session_);
        session_begun_ = false;
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
        OpenXrInstanceLifetime::unregisterInstanceUser(instance_);
        xrDestroyInstance(instance_);
        instance_ = XR_NULL_HANDLE;
    }
    session_running_ = false;
}

void OpenXrSession::handleSessionStateChanged(XrSessionState xr_state) {
    switch (xr_state) {
        case XR_SESSION_STATE_IDLE:
            state_ = SessionState::eIdle;
            session_running_ = false;
            break;
        case XR_SESSION_STATE_READY: {
            state_ = SessionState::eReady;
            if (!session_begun_) {
                XrSessionBeginInfo begin{XR_TYPE_SESSION_BEGIN_INFO};
                begin.primaryViewConfigurationType = view_configuration_;
                if (XR_SUCCEEDED(xrBeginSession(session_, &begin))) {
                    session_begun_ = true;
                }
            }
            break;
        }
        case XR_SESSION_STATE_SYNCHRONIZED:
            state_ = SessionState::eSynchronized;
            session_running_ = true;
            break;
        case XR_SESSION_STATE_VISIBLE:
            state_ = SessionState::eVisible;
            session_running_ = true;
            break;
        case XR_SESSION_STATE_FOCUSED:
            state_ = SessionState::eFocused;
            session_running_ = true;
            break;
        case XR_SESSION_STATE_STOPPING:
            state_ = SessionState::eStopping;
            session_running_ = false;
            if (session_begun_) {
                xrEndSession(session_);
                session_begun_ = false;
            }
            swapchain_bridge_.destroy();
            swapchains_ready_ = false;
            running_ = false;
            break;
        case XR_SESSION_STATE_LOSS_PENDING:
            state_ = SessionState::eLossPending;
            session_running_ = false;
            break;
        case XR_SESSION_STATE_EXITING:
            state_ = SessionState::eExiting;
            session_running_ = false;
            running_ = false;
            break;
        default:
            break;
    }
}

bool OpenXrSession::ensureSwapchains() {
    if (swapchains_ready_) {
        return true;
    }
    if (view_configs_.empty() || color_format_ == 0) {
        return false;
    }
    if (!swapchain_bridge_.create(session_, view_configs_, color_format_, depth_format_)) {
        return false;
    }
    swapchains_ready_ = true;
    return true;
}

void OpenXrSession::flushGpu() {
    if (auto device = graphics_.getDevice()) {
        if (auto* vk_device = dynamic_cast<vne::rhi::VulkanDevice*>(device.get())) {
            const vk::Device dev = vk_device->getVulkanDevice();
            if (dev) {
                dev.waitIdle();
            }
        }
    }
}

bool OpenXrSession::pollEvents() {
    if (!running_ || instance_ == XR_NULL_HANDLE) {
        return false;
    }
    XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
    while (xrPollEvent(instance_, &event) == XR_SUCCESS) {
        switch (event.type) {
            case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
                const auto* state_event = reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
                handleSessionStateChanged(state_event->state);
                break;
            }
            default:
                break;
        }
        event = {XR_TYPE_EVENT_DATA_BUFFER};
    }
    return running_;
}

bool OpenXrSession::beginFrame(Frame& out_frame) {
    if (!running_ || session_ == XR_NULL_HANDLE || !session_running_) {
        return false;
    }

    XrFrameWaitInfo wait_info{XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frame_state{XR_TYPE_FRAME_STATE};
    if (XR_FAILED(xrWaitFrame(session_, &wait_info, &frame_state))) {
        return false;
    }
    XrFrameBeginInfo begin_info{XR_TYPE_FRAME_BEGIN_INFO};
    if (XR_FAILED(xrBeginFrame(session_, &begin_info))) {
        return false;
    }

    predicted_display_time_ = frame_state.predictedDisplayTime;
    out_frame = {};
    out_frame.frame_index = frame_index_++;
    out_frame.predicted_display_time = static_cast<double>(frame_state.predictedDisplayTime);
    out_frame.should_render = frame_state.shouldRender == XR_TRUE;

    if (!out_frame.should_render) {
        return true;
    }

    XrViewLocateInfo locate_info{XR_TYPE_VIEW_LOCATE_INFO};
    locate_info.viewConfigurationType = view_configuration_;
    locate_info.displayTime = frame_state.predictedDisplayTime;
    locate_info.space = reference_space_;

    XrViewState view_state{XR_TYPE_VIEW_STATE};
    constexpr std::uint32_t kCapacity = 2;
    std::array<XrView, kCapacity> xr_views{};
    std::uint32_t view_count = kCapacity;
    if (XR_SUCCEEDED(xrLocateViews(session_, &locate_info, &view_state, view_count, &view_count, xr_views.data()))) {
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
        if (ensureSwapchains()) {
            swapchain_bridge_.acquireForFrame(out_frame);
        }
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

    flushGpu();

    if (swapchains_ready_) {
        swapchain_bridge_.releaseForFrame();
    }

    XrFrameEndInfo end_info{XR_TYPE_FRAME_END_INFO};
    end_info.displayTime = static_cast<XrTime>(frame.predicted_display_time);
    end_info.environmentBlendMode = blend_mode_;

    XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    std::array<XrCompositionLayerProjectionView, 2> projection_views{};
    if (frame.should_render && frame.view_count > 0 && swapchains_ready_) {
        composition_layer_.buildProjectionLayer(reference_space_,
                                                swapchain_bridge_,
                                                frame.view_count,
                                                layer,
                                                projection_views);
        const XrCompositionLayerBaseHeader* layer_hdr = reinterpret_cast<const XrCompositionLayerBaseHeader*>(&layer);
        end_info.layerCount = 1;
        end_info.layers = &layer_hdr;
    } else {
        end_info.layerCount = 0;
        end_info.layers = nullptr;
    }
    xrEndFrame(session_, &end_info);
}

}  // namespace vne::xr
