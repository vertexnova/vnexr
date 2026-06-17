#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   June 2026
 *
 * Autodoc:   yes
 * ---------------------------------------------------------------------- */

/**
 * @file session.h
 * @brief Platform-agnostic XR session lifecycle.
 */

#include "vertexnova/xr/frame.h"
#include "vertexnova/xr/input.h"
#include "vertexnova/xr/render_session.h"
#include "vertexnova/xr/types.h"

#include <memory>
#include <string>

namespace vne::xr {

struct SessionConfig {
    BackendType backend = BackendType::eNull;
    std::string application_name = "VneXR";
};

/**
 * @brief Platform-agnostic XR session lifecycle.
 *
 * Backends: Null (CI), OpenXR (Android/Windows), CompositorServices (visionOS).
 */
class VNE_XR_API ISession {
   public:
    virtual ~ISession() = default;

    [[nodiscard]] virtual BackendType backendType() const = 0;
    [[nodiscard]] virtual SessionState state() const = 0;

    /** @brief Poll runtime events; return false to exit main loop. */
    virtual bool pollEvents() = 0;

    /** @brief Advance to next frame; fills @p out_frame. */
    virtual bool beginFrame(Frame& out_frame) = 0;

    /** @brief Submit composition layers for @p frame. */
    virtual void endFrame(const Frame& frame, const LayerParams& layers) = 0;

    /** @brief Optional input snapshot after beginFrame. */
    virtual void pollInput(InputState& out_input) { (void)out_input; }

    /** @brief Run loop with app render session until pollEvents returns false. */
    virtual void run(IRenderSession& app_session);
};

/** @brief Create session for @p config (factory dispatches by backend). */
[[nodiscard]] VNE_XR_API std::unique_ptr<ISession> createSession(const SessionConfig& config);

}  // namespace vne::xr
