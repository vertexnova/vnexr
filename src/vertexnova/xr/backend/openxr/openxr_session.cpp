/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "vertexnova/xr/backend/openxr/openxr_session.h"

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

#ifdef VNE_XR_WITH_RHI
#include "vertexnova/rhi/graphics_factory.h"
#endif

namespace vne::xr_ns {

namespace {

XrPosef identity_pose() {
    XrPosef pose{};
    pose.orientation.w = 1.f;
    return pose;
}

void store_pose(const XrPosef& src, Pose& dst) {
    dst.position[0] = src.position.x;
    dst.position[1] = src.position.y;
    dst.position[2] = src.position.z;
    dst.orientation[0] = src.orientation.x;
    dst.orientation[1] = src.orientation.y;
    dst.orientation[2] = src.orientation.z;
    dst.orientation[3] = src.orientation.w;
}

void store_fov(const XrFovf& src, FieldOfView& dst) {
    dst.angle_left = src.angleLeft;
    dst.angle_right = src.angleRight;
    dst.angle_up = src.angleUp;
    dst.angle_down = src.angleDown;
}

}  // namespace

OpenXrSession::OpenXrSession(SessionConfig config)
    : config_(std::move(config)) {
    if (!init_openxr()) {
        state_ = SessionState::eExiting;
        running_ = false;
    }
}

OpenXrSession::~OpenXrSession() {
    shutdown_openxr();
}

BackendType OpenXrSession::backend_type() const {
    return BackendType::eOpenXr;
}

SessionState OpenXrSession::state() const {
    return state_;
}

bool OpenXrSession::init_openxr() {
    XrInstanceCreateInfo create_info{XR_TYPE_INSTANCE_CREATE_INFO};
    create_info.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    std::strncpy(create_info.applicationInfo.applicationName,
                 config_.application_name.c_str(),
                 XR_MAX_APPLICATION_NAME_SIZE - 1);

    std::vector<const char*> extensions;
#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN)
    extensions.push_back(XR_KHR_VULKAN_ENABLE_EXTENSION_NAME);
#endif
#if defined(__ANDROID__)
    extensions.push_back(XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME);
#endif
    extensions.push_back(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME);

    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.enabledExtensionNames = extensions.data();

    XrInstance instance = XR_NULL_HANDLE;
    if (XR_FAILED(xrCreateInstance(&create_info, &instance))) {
        return false;
    }
    instance_ = instance;

    XrSystemGetInfo system_info{XR_TYPE_SYSTEM_GET_INFO};
    system_info.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrSystemId system_id = XR_NULL_SYSTEM_ID;
    if (XR_FAILED(xrGetSystem(instance, &system_info, &system_id))) {
        return false;
    }
    system_id_ = reinterpret_cast<void*>(static_cast<std::uintptr_t>(system_id));

    XrSessionCreateInfo session_info{XR_TYPE_SESSION_CREATE_INFO};
    session_info.systemId = system_id;

#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN) && defined(VNE_XR_WITH_RHI)
    XrGraphicsBindingVulkanKHR vulkan_binding{XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR};
    const auto backend = vne::rhi::GraphicsFactory::getDefaultBackend();
    (void)backend;
    vulkan_binding.instance = VK_NULL_HANDLE;
    vulkan_binding.physicalDevice = VK_NULL_HANDLE;
    vulkan_binding.device = VK_NULL_HANDLE;
    vulkan_binding.queueFamilyIndex = 0;
    vulkan_binding.queueIndex = 0;
    session_info.next = &vulkan_binding;
#endif

    XrSession session = XR_NULL_HANDLE;
    if (XR_FAILED(xrCreateSession(instance, &session_info, &session))) {
        return false;
    }
    session_ = session;

    XrReferenceSpaceCreateInfo space_info{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    space_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    space_info.poseInReferenceSpace = identity_pose();
    XrSpace reference_space = XR_NULL_HANDLE;
    if (XR_FAILED(xrCreateReferenceSpace(session, &space_info, &reference_space))) {
        return false;
    }
    reference_space_ = reference_space;

    state_ = SessionState::eReady;
    return true;
}

void OpenXrSession::shutdown_openxr() {
    swapchain_bridge_.destroy();
    swapchains_ready_ = false;

    if (reference_space_ != nullptr) {
        xrDestroySpace(static_cast<XrSpace>(reference_space_));
        reference_space_ = nullptr;
    }
    if (session_ != nullptr) {
        xrDestroySession(static_cast<XrSession>(session_));
        session_ = nullptr;
    }
    if (instance_ != nullptr) {
        xrDestroyInstance(static_cast<XrInstance>(instance_));
        instance_ = nullptr;
    }
}

bool OpenXrSession::ensure_swapchains(std::uint32_t view_count) {
    if (swapchains_ready_) {
        return true;
    }
    if (view_count == 0) {
        return false;
    }
    if (swapchain_width_ == 0 || swapchain_height_ == 0) {
        swapchain_width_ = 1024;
        swapchain_height_ = 1024;
    }
    if (!swapchain_bridge_.create(session_, view_count, swapchain_width_, swapchain_height_)) {
        return false;
    }
    swapchains_ready_ = true;
    return true;
}

bool OpenXrSession::poll_events() {
    if (!running_ || instance_ == nullptr) {
        return false;
    }
    XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
    while (xrPollEvent(static_cast<XrInstance>(instance_), &event) == XR_SUCCESS) {
        switch (event.type) {
            case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
                const auto* state_event = reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
                switch (state_event->state) {
                    case XR_SESSION_STATE_READY:
                        state_ = SessionState::eReady;
                        xrBeginSession(static_cast<XrSession>(session_), nullptr);
                        break;
                    case XR_SESSION_STATE_SYNCHRONIZED:
                        state_ = SessionState::eSynchronized;
                        break;
                    case XR_SESSION_STATE_VISIBLE:
                        state_ = SessionState::eVisible;
                        break;
                    case XR_SESSION_STATE_FOCUSED:
                        state_ = SessionState::eFocused;
                        break;
                    case XR_SESSION_STATE_STOPPING:
                        state_ = SessionState::eStopping;
                        running_ = false;
                        break;
                    case XR_SESSION_STATE_EXITING:
                        state_ = SessionState::eExiting;
                        running_ = false;
                        break;
                    default:
                        break;
                }
                break;
            }
            default:
                break;
        }
        event = {XR_TYPE_EVENT_DATA_BUFFER};
    }
    return running_;
}

bool OpenXrSession::begin_frame(Frame& out_frame) {
    if (!running_ || session_ == nullptr) {
        return false;
    }
    XrFrameWaitInfo wait_info{XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frame_state{XR_TYPE_FRAME_STATE};
    if (XR_FAILED(xrWaitFrame(static_cast<XrSession>(session_), &wait_info, &frame_state))) {
        return false;
    }
    XrFrameBeginInfo begin_info{XR_TYPE_FRAME_BEGIN_INFO};
    if (XR_FAILED(xrBeginFrame(static_cast<XrSession>(session_), &begin_info))) {
        return false;
    }

    out_frame = {};
    out_frame.frame_index = frame_index_++;
    out_frame.predicted_display_time = static_cast<double>(frame_state.predictedDisplayTime);
    out_frame.should_render = frame_state.shouldRender == XR_TRUE;

    if (!out_frame.should_render) {
        return true;
    }

    XrViewLocateInfo locate_info{XR_TYPE_VIEW_LOCATE_INFO};
    locate_info.displayTime = frame_state.predictedDisplayTime;
    locate_info.space = static_cast<XrSpace>(reference_space_);

    XrViewState view_state{XR_TYPE_VIEW_STATE};
    constexpr uint32_t kCapacity = 2;
    std::array<XrView, kCapacity> xr_views{};
    uint32_t view_count = kCapacity;
    if (XR_SUCCEEDED(xrLocateViews(static_cast<XrSession>(session_),
                                   &locate_info,
                                   &view_state,
                                   view_count,
                                   &view_count,
                                   xr_views.data()))) {
        out_frame.view_count = view_count;
        out_frame.surfaces.view_count = view_count;
        for (uint32_t i = 0; i < view_count && i < Frame::kMaxViews; ++i) {
            out_frame.views[i].view_index = i;
            store_pose(xr_views[i].pose, out_frame.views[i].pose);
            store_fov(xr_views[i].fov, out_frame.views[i].fov);
            out_frame.surfaces.views[i].view_index = i;
        }
        if (ensure_swapchains(view_count)) {
            swapchain_bridge_.acquire_for_frame(out_frame);
        }
    }
    return true;
}

void OpenXrSession::end_frame(const Frame& frame, const LayerParams& layers) {
    (void)layers;
    if (session_ == nullptr) {
        return;
    }

    if (swapchains_ready_) {
        swapchain_bridge_.release_for_frame();
    }

    XrFrameEndInfo end_info{XR_TYPE_FRAME_END_INFO};
    end_info.displayTime = static_cast<XrTime>(frame.predicted_display_time);
    end_info.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;

    XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    layer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
    layer.space = static_cast<XrSpace>(reference_space_);
    std::array<XrCompositionLayerProjectionView, 2> projection_views{};
    if (frame.should_render && frame.view_count > 0) {
        for (std::uint32_t i = 0; i < frame.view_count && i < 2; ++i) {
            projection_views[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
            projection_views[i].pose = identity_pose();
            projection_views[i].fov = {-0.5f, 0.5f, 0.5f, -0.5f};
            projection_views[i].subImage.swapchain = XR_NULL_HANDLE;
        }
        layer.viewCount = frame.view_count;
        layer.views = projection_views.data();
        const XrCompositionLayerBaseHeader* layers_hdr = reinterpret_cast<const XrCompositionLayerBaseHeader*>(&layer);
        end_info.layerCount = 1;
        end_info.layers = &layers_hdr;
    } else {
        end_info.layerCount = 0;
        end_info.layers = nullptr;
    }
    xrEndFrame(static_cast<XrSession>(session_), &end_info);
}

}  // namespace vne::xr_ns
