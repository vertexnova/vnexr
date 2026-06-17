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

#include "stereo_cube_scene.h"
#include "vertexnova/xr/render_session.h"

namespace vne::xr::examples {

/** @brief Per-eye stereo cube demo with Khronos Ch.4 block interaction. */
class StereoCubeRenderer final : public IRenderSession {
   public:
    void onSessionReady() override;
    void update(const FrameParams& params, LayerParams& out_layers) override;

   private:
    void clearEyeSurface(const ViewSurface& surface, float r, float g, float b);

    StereoCubeScene scene_;
    std::uint64_t frames_rendered_ = 0;
#ifdef VNE_XR_WITH_RHI
    std::shared_ptr<vne::rhi::IDevice> device_;
#endif
};

}  // namespace vne::xr::examples
