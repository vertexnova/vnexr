#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "vertexnova/xr/frame.h"
#include "vertexnova/xr/render_session.h"

#include <string>

namespace vne::xr::samples {

/**
 * @brief Base class for XR sample layers (scene, input, debug overlays).
 *
 * Mirrors vnegfx/vnerhi ILayer but driven by vne::xr::FrameParams each XR frame.
 */
class IXrLayer {
   public:
    explicit IXrLayer(std::string name = "XrLayer");
    virtual ~IXrLayer() = default;

    IXrLayer(const IXrLayer&) = delete;
    IXrLayer& operator=(const IXrLayer&) = delete;

    virtual void onAttach() {}
    virtual void onDetach() {}

    /** Called once per XR frame before per-eye rendering hooks. */
    virtual void onUpdate(const FrameParams& params, LayerParams& out_layers) {
        (void)params;
        (void)out_layers;
    }

    void setEnabled(bool enabled) { enabled_ = enabled; }
    [[nodiscard]] bool isEnabled() const { return enabled_; }

    [[nodiscard]] const std::string& name() const { return name_; }

   private:
    bool enabled_ = true;
    std::string name_;
};

}  // namespace vne::xr::samples
