#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "layers/xr_layer.h"

#include <memory>
#include <vector>

namespace vne::xr::samples {

/** @brief Ordered XR layer stack (scene layers, then overlays). */
class XrLayerStack {
   public:
    void pushLayer(std::unique_ptr<IXrLayer> layer);
    void pushOverlay(std::unique_ptr<IXrLayer> overlay);

    void onAttach();
    void onDetach();
    void onUpdate(const FrameParams& params, LayerParams& out_layers);

    [[nodiscard]] std::size_t layerCount() const;

   private:
    std::vector<std::unique_ptr<IXrLayer>> layers_;
    std::vector<std::unique_ptr<IXrLayer>> overlays_;
};

}  // namespace vne::xr::samples
