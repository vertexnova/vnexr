#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "layers/xr_layer.h"

#include <memory>
#include <vector>

namespace vne::xr::samples {

class TextRenderer;

/** @brief Ordered XR layer stack (scene layers, then overlays). */
class XrLayerStack {
   public:
    void pushLayer(std::unique_ptr<IXrLayer> layer);
    void pushOverlay(std::unique_ptr<IXrLayer> overlay);

    void onAttach(ISession* session);
    void onDetach();
    void onUpdate(const FrameParams& params, LayerParams& out_layers);

    /** @brief Forward to every enabled layer, then every enabled overlay. */
    void onRenderEye(const EyeRenderContext& context);

    [[nodiscard]] std::size_t layerCount() const;

    /** @brief Text renderer shared by the host, for layers that draw labels; set before the sample's `setup`. Null in
     * tests. */
    void setTextRenderer(TextRenderer* text) { text_ = text; }
    [[nodiscard]] TextRenderer* textRenderer() const { return text_; }

   private:
    std::vector<std::unique_ptr<IXrLayer>> layers_;
    std::vector<std::unique_ptr<IXrLayer>> overlays_;
    TextRenderer* text_ = nullptr;
};

}  // namespace vne::xr::samples
