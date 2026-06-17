/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "vertexnova/xr/session.h"

#include "vertexnova/xr/core/null_session.h"

#if defined(VNE_XR_WITH_OPENXR) && VNE_XR_WITH_OPENXR
#include "vertexnova/xr/backend/openxr/openxr_session.h"
#endif

#if defined(VNE_XR_WITH_VISIONOS) && VNE_XR_WITH_VISIONOS
#include "vertexnova/xr/backend/visionos/visionos_session.h"
#endif

namespace vne::xr_ns {

void ISession::run(IRenderSession& app_session) {
    app_session.on_session_ready();
    Frame frame{};
    LayerParams layers{};
    while (poll_events()) {
        if (!begin_frame(frame)) {
            continue;
        }
        FrameParams params{frame};
        app_session.update(params, layers);
        end_frame(frame, layers);
        if (layers.request_exit) {
            break;
        }
    }
}

std::unique_ptr<ISession> create_session(const SessionConfig& config) {
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

}  // namespace vne::xr_ns
