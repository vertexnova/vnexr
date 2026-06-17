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
 * @file render_session.h
 * @brief Application render callback invoked each XR frame.
 */

#include "vertexnova/xr/frame.h"

namespace vne::xr {

/**
 * @brief Application render callback invoked each XR frame.
 *
 * Implementations receive compositor-owned surfaces and per-view matrices.
 */
class VNE_XR_API IRenderSession {
   public:
    virtual ~IRenderSession() = default;

    /** @brief One-time setup after session becomes ready. */
    virtual void onSessionReady() {}

    /** @brief Per-frame content render hook. */
    virtual void update(const FrameParams& params, LayerParams& out_layers) = 0;
};

}  // namespace vne::xr
