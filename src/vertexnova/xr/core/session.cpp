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

#include "vertexnova/xr/session.h"

#include "vertexnova/xr/core/null_session.h"

#if defined(VNE_XR_WITH_OPENXR) && VNE_XR_WITH_OPENXR
#include "vertexnova/xr/backend/openxr/openxr_session.h"
#endif

#if defined(VNE_XR_WITH_VISIONOS) && VNE_XR_WITH_VISIONOS
#include "vertexnova/xr/backend/visionos/visionos_session.h"
#endif

namespace vne::xr {

void ISession::run(IRenderSession& app_session) {
    app_session.onSessionReady();
    Frame frame{};
    LayerParams layers{};
    InputState input{};
    while (pollEvents()) {
        if (!beginFrame(frame)) {
            continue;
        }
        pollInput(input);
        FrameParams params{frame};
        params.input = input;
        app_session.update(params, layers);
        endFrame(frame, layers);
        if (layers.request_exit) {
            break;
        }
    }
}

std::unique_ptr<ISession> createSession(const SessionConfig& config) {
    switch (config.backend) {
#if defined(VNE_XR_WITH_OPENXR) && VNE_XR_WITH_OPENXR
        case BackendType::eOpenXr:
            return std::make_unique<OpenXrSession>(config);
#endif
#if defined(VNE_XR_WITH_VISIONOS) && VNE_XR_WITH_VISIONOS
        case BackendType::eVisionOs:
            return std::make_unique<VisionOsSession>(config);
#endif
        case BackendType::eNull:
        default:
            return std::make_unique<NullSession>(config);
    }
}

}  // namespace vne::xr
