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

#include "vertexnova/xr/xr_factory.h"

#include <sstream>

namespace vne::xr {

std::vector<BackendType> XrFactory::getAvailableBackends() {
    std::vector<BackendType> backends{BackendType::eNull};
#if defined(VNE_XR_WITH_OPENXR) && VNE_XR_WITH_OPENXR
    backends.push_back(BackendType::eOpenXr);
#endif
#if defined(VNE_XR_WITH_VISIONOS) && VNE_XR_WITH_VISIONOS
    backends.push_back(BackendType::eVisionOs);
#endif
    return backends;
}

bool XrFactory::isBackendAvailable(BackendType backend) {
    for (const auto available : getAvailableBackends()) {
        if (available == backend) {
            return true;
        }
    }
    return false;
}

BackendType XrFactory::getDefaultBackend() {
#if defined(VNE_XR_WITH_VISIONOS) && VNE_XR_WITH_VISIONOS
    return BackendType::eVisionOs;
#elif defined(VNE_XR_WITH_OPENXR) && VNE_XR_WITH_OPENXR
    return BackendType::eOpenXr;
#else
    return BackendType::eNull;
#endif
}

BackendType XrFactory::selectBestBackend(BackendType preferred) {
    if (preferred != BackendType::eNull && isBackendAvailable(preferred)) {
        return preferred;
    }
    return getDefaultBackend();
}

std::string XrFactory::backendToString(BackendType backend) {
    switch (backend) {
        case BackendType::eNull:
            return "Null";
        case BackendType::eOpenXr:
            return "OpenXR";
        case BackendType::eVisionOs:
            return "visionOS";
    }
    return "Unknown";
}

std::string XrFactory::getAvailableBackendsString() {
    const auto backends = getAvailableBackends();
    std::ostringstream oss;
    for (std::size_t i = 0; i < backends.size(); ++i) {
        if (i > 0) {
            oss << ", ";
        }
        oss << backendToString(backends[i]);
    }
    return oss.str();
}

}  // namespace vne::xr
