#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "layers/xr_layer.h"

#include "vertexnova/gfx/engine/gfx_session.h"
#include "vertexnova/rhi/device.h"

#include <memory>

namespace vne::xr::samples {

/**
 * @brief Stretch sample: drive vnegfx GfxSession from the XR host device.
 *
 * Preferred path: bind per-eye RTs from wrapped XR ITexture surfaces.
 * Fallback: render CSG offscreen on the same IDevice, then copy into eye images.
 */
class CsgXrSceneLayer final : public IXrLayer {
   public:
    CsgXrSceneLayer();
    ~CsgXrSceneLayer() override;

    void onAttach(ISession* session) override;
    void onDetach() override;
    void onUpdate(const FrameParams& params, LayerParams& out_layers) override;

   private:
    std::shared_ptr<vne::rhi::IDevice> device_;
    std::unique_ptr<vne::gfx::GfxSession> gfx_session_;
    std::uint64_t frames_ = 0;
    bool warned_missing_surfaces_ = false;
};

}  // namespace vne::xr::samples
