#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "layers/xr_layer.h"

#include <cstdint>

namespace vne::xr::samples {

/** @brief Logs frame info and requests exit after N frames (headless XR simulator). */
class ExitAfterFramesLayer final : public IXrLayer {
   public:
    explicit ExitAfterFramesLayer(std::uint64_t max_frames = 3);

    void onUpdate(const FrameParams& params, LayerParams& out_layers) override;

   private:
    std::uint64_t max_frames_;
};

}  // namespace vne::xr::samples
