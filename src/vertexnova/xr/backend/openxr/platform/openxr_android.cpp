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

#include "vertexnova/xr/backend/openxr/platform/openxr_android.h"

#if defined(__ANDROID__)
#include <openxr/openxr_platform.h>
#endif

#include "vertexnova/logging/logging.h"
#include "vertexnova/xr/backend/openxr/openxr_util.h"

namespace vne::xr {

Result initializeOpenXrLoaderAndroid(void* application_vm, void* application_context) {
#if !defined(__ANDROID__)
    (void)application_vm;
    (void)application_context;
    return Result::eErrorNotSupported;
#else
    if (application_vm == nullptr || application_context == nullptr) {
        return Result::eErrorInvalidArgument;
    }

    XrLoaderInitInfoAndroidKHR loader_info{XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
    loader_info.applicationVM = application_vm;
    loader_info.applicationContext = application_context;

    PFN_xrInitializeLoaderKHR pfn_init = nullptr;
    if (XR_FAILED(xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
                                      reinterpret_cast<PFN_xrVoidFunction*>(&pfn_init))) ||
        pfn_init == nullptr) {
        VNE_LOG_ERROR << "xrInitializeLoaderKHR not available";
        return Result::eErrorOpenXrFailed;
    }
    OPENXR_CHECK(pfn_init(reinterpret_cast<const XrLoaderInitInfoBaseHeader*>(&loader_info)));
    VNE_LOG_INFO << "OpenXR Android loader initialized";
    return Result::eSuccess;
#endif
}

}  // namespace vne::xr
