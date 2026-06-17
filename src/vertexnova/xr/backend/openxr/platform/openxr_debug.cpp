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

#include "vertexnova/xr/backend/openxr/platform/openxr_debug.h"

#include "vertexnova/logging/logging.h"

namespace vne::xr {

namespace {

XrBool32 XRAPI_PTR debugCallback(XrDebugUtilsMessageSeverityFlagsEXT severity,
                                 XrDebugUtilsMessageTypeFlagsEXT types,
                                 const XrDebugUtilsMessengerCallbackDataEXT* callback_data,
                                 void* user_data) {
    (void)types;
    (void)user_data;
    if (callback_data == nullptr) {
        return XR_FALSE;
    }
    if ((severity & XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0) {
        VNE_LOG_ERROR << "[OpenXR] " << callback_data->message;
    } else if ((severity & XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0) {
        VNE_LOG_WARN << "[OpenXR] " << callback_data->message;
    } else {
        VNE_LOG_INFO << "[OpenXR] " << callback_data->message;
    }
    return XR_FALSE;
}

}  // namespace

bool OpenXrDebugMessenger::create(XrInstance instance, PFN_xrGetInstanceProcAddr get_proc) {
    destroy(instance);
    auto pfn_create =
        reinterpret_cast<PFN_xrCreateDebugUtilsMessengerEXT>(get_proc(instance, "xrCreateDebugUtilsMessengerEXT"));
    if (pfn_create == nullptr) {
        return false;
    }
    XrDebugUtilsMessengerCreateInfoEXT info{XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    info.messageSeverities =
        XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageTypes = XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                        | XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.userCallback = debugCallback;
    return XR_SUCCEEDED(pfn_create(instance, &info, &messenger_));
}

void OpenXrDebugMessenger::destroy(XrInstance instance) {
    if (messenger_ == XR_NULL_HANDLE || instance == XR_NULL_HANDLE) {
        return;
    }
    auto pfn_destroy = reinterpret_cast<PFN_xrDestroyDebugUtilsMessengerEXT>(
        xrGetInstanceProcAddr(instance, "xrDestroyDebugUtilsMessengerEXT"));
    if (pfn_destroy != nullptr) {
        pfn_destroy(messenger_);
    }
    messenger_ = XR_NULL_HANDLE;
}

}  // namespace vne::xr
