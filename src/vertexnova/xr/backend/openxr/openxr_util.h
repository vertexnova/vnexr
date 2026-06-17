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

#include <openxr/openxr.h>

#include "vertexnova/logging/logging.h"
#include "vertexnova/xr/xr_error.h"

#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace vne::xr::OpenXrUtil {

[[nodiscard]] inline std::string_view xrResultToString(XrResult result) {
    switch (result) {
        case XR_SUCCESS:
            return "XR_SUCCESS";
        case XR_TIMEOUT_EXPIRED:
            return "XR_TIMEOUT_EXPIRED";
        case XR_SESSION_LOSS_PENDING:
            return "XR_SESSION_LOSS_PENDING";
        case XR_EVENT_UNAVAILABLE:
            return "XR_EVENT_UNAVAILABLE";
        case XR_SPACE_BOUNDS_UNAVAILABLE:
            return "XR_SPACE_BOUNDS_UNAVAILABLE";
        case XR_SESSION_NOT_FOCUSED:
            return "XR_SESSION_NOT_FOCUSED";
        case XR_FRAME_DISCARDED:
            return "XR_FRAME_DISCARDED";
        case XR_ERROR_VALIDATION_FAILURE:
            return "XR_ERROR_VALIDATION_FAILURE";
        case XR_ERROR_RUNTIME_FAILURE:
            return "XR_ERROR_RUNTIME_FAILURE";
        case XR_ERROR_OUT_OF_MEMORY:
            return "XR_ERROR_OUT_OF_MEMORY";
        case XR_ERROR_API_VERSION_UNSUPPORTED:
            return "XR_ERROR_API_VERSION_UNSUPPORTED";
        case XR_ERROR_INITIALIZATION_FAILED:
            return "XR_ERROR_INITIALIZATION_FAILED";
        case XR_ERROR_FUNCTION_UNSUPPORTED:
            return "XR_ERROR_FUNCTION_UNSUPPORTED";
        case XR_ERROR_FEATURE_UNSUPPORTED:
            return "XR_ERROR_FEATURE_UNSUPPORTED";
        case XR_ERROR_EXTENSION_NOT_PRESENT:
            return "XR_ERROR_EXTENSION_NOT_PRESENT";
        case XR_ERROR_LIMIT_REACHED:
            return "XR_ERROR_LIMIT_REACHED";
        case XR_ERROR_SIZE_INSUFFICIENT:
            return "XR_ERROR_SIZE_INSUFFICIENT";
        case XR_ERROR_HANDLE_INVALID:
            return "XR_ERROR_HANDLE_INVALID";
        case XR_ERROR_INSTANCE_LOST:
            return "XR_ERROR_INSTANCE_LOST";
        case XR_ERROR_SESSION_RUNNING:
            return "XR_ERROR_SESSION_RUNNING";
        case XR_ERROR_SESSION_NOT_RUNNING:
            return "XR_ERROR_SESSION_NOT_RUNNING";
        case XR_ERROR_SESSION_LOST:
            return "XR_ERROR_SESSION_LOST";
        case XR_ERROR_SYSTEM_INVALID:
            return "XR_ERROR_SYSTEM_INVALID";
        case XR_ERROR_PATH_INVALID:
            return "XR_ERROR_PATH_INVALID";
        case XR_ERROR_PATH_COUNT_EXCEEDED:
            return "XR_ERROR_PATH_COUNT_EXCEEDED";
        case XR_ERROR_PATH_FORMAT_INVALID:
            return "XR_ERROR_PATH_FORMAT_INVALID";
        case XR_ERROR_PATH_UNSUPPORTED:
            return "XR_ERROR_PATH_UNSUPPORTED";
        case XR_ERROR_LAYER_INVALID:
            return "XR_ERROR_LAYER_INVALID";
        case XR_ERROR_LAYER_LIMIT_EXCEEDED:
            return "XR_ERROR_LAYER_LIMIT_EXCEEDED";
        case XR_ERROR_SWAPCHAIN_RECT_INVALID:
            return "XR_ERROR_SWAPCHAIN_RECT_INVALID";
        case XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED:
            return "XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED";
        case XR_ERROR_ACTION_TYPE_MISMATCH:
            return "XR_ERROR_ACTION_TYPE_MISMATCH";
        case XR_ERROR_SESSION_NOT_READY:
            return "XR_ERROR_SESSION_NOT_READY";
        case XR_ERROR_SESSION_NOT_STOPPING:
            return "XR_ERROR_SESSION_NOT_STOPPING";
        case XR_ERROR_TIME_INVALID:
            return "XR_ERROR_TIME_INVALID";
        case XR_ERROR_REFERENCE_SPACE_UNSUPPORTED:
            return "XR_ERROR_REFERENCE_SPACE_UNSUPPORTED";
        case XR_ERROR_FILE_ACCESS_ERROR:
            return "XR_ERROR_FILE_ACCESS_ERROR";
        case XR_ERROR_FILE_CONTENTS_INVALID:
            return "XR_ERROR_FILE_CONTENTS_INVALID";
        case XR_ERROR_FORM_FACTOR_UNSUPPORTED:
            return "XR_ERROR_FORM_FACTOR_UNSUPPORTED";
        case XR_ERROR_FORM_FACTOR_UNAVAILABLE:
            return "XR_ERROR_FORM_FACTOR_UNAVAILABLE";
        case XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED:
            return "XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED";
        case XR_ERROR_ENVIRONMENT_BLEND_MODE_UNSUPPORTED:
            return "XR_ERROR_ENVIRONMENT_BLEND_MODE_UNSUPPORTED";
        default:
            return "XR_UNKNOWN";
    }
}

[[nodiscard]] inline Result fromXrResult(XrResult xr_result) {
    if (XR_SUCCEEDED(xr_result)) {
        return Result::eSuccess;
    }
    return Result::eErrorOpenXrFailed;
}

[[nodiscard]] inline std::vector<XrExtensionProperties> enumerateInstanceExtensions() {
    std::uint32_t count = 0;
    if (XR_FAILED(xrEnumerateInstanceExtensionProperties(nullptr, 0, &count, nullptr))) {
        return {};
    }
    std::vector<XrExtensionProperties> extensions(count, {XR_TYPE_EXTENSION_PROPERTIES});
    if (XR_FAILED(xrEnumerateInstanceExtensionProperties(nullptr, count, &count, extensions.data()))) {
        return {};
    }
    return extensions;
}

[[nodiscard]] inline bool isExtensionAvailable(std::string_view name,
                                               const std::vector<XrExtensionProperties>& available) {
    for (const auto& ext : available) {
        if (name == ext.extensionName) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] inline std::vector<const char*> filterEnabledExtensions(
    const std::vector<std::string>& requested, const std::vector<XrExtensionProperties>& available) {
    std::vector<const char*> enabled;
    enabled.reserve(requested.size());
    for (const auto& name : requested) {
        if (isExtensionAvailable(name, available)) {
            enabled.push_back(name.c_str());
        } else {
            VNE_LOG_WARN << "OpenXR extension not available: " << name;
        }
    }
    return enabled;
}

}  // namespace vne::xr::OpenXrUtil

#define OPENXR_CHECK(expr)                                                                                             \
    do {                                                                                                               \
        const XrResult _openxr_result = (expr);                                                                        \
        if (XR_FAILED(_openxr_result)) {                                                                               \
            VNE_LOG_ERROR << "OpenXR call failed: " << ::vne::xr::OpenXrUtil::xrResultToString(_openxr_result) << " (" \
                          << static_cast<int>(_openxr_result) << ")";                                                  \
            return ::vne::xr::Result::eErrorOpenXrFailed;                                                              \
        }                                                                                                              \
    } while (0)

#define OPENXR_CHECK_BOOL(expr)                                                                                        \
    do {                                                                                                               \
        const XrResult _openxr_result = (expr);                                                                        \
        if (XR_FAILED(_openxr_result)) {                                                                               \
            VNE_LOG_ERROR << "OpenXR call failed: " << ::vne::xr::OpenXrUtil::xrResultToString(_openxr_result) << " (" \
                          << static_cast<int>(_openxr_result) << ")";                                                  \
            return false;                                                                                              \
        }                                                                                                              \
    } while (0)
