/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "layers/xr_layer_stack.h"

namespace vne::xr::samples {

void XrLayerStack::pushLayer(std::unique_ptr<IXrLayer> layer) {
    layers_.push_back(std::move(layer));
}

void XrLayerStack::pushOverlay(std::unique_ptr<IXrLayer> overlay) {
    overlays_.push_back(std::move(overlay));
}

void XrLayerStack::onAttach() {
    for (auto& layer : layers_) {
        if (layer && layer->isEnabled()) {
            layer->onAttach();
        }
    }
    for (auto& overlay : overlays_) {
        if (overlay && overlay->isEnabled()) {
            overlay->onAttach();
        }
    }
}

void XrLayerStack::onDetach() {
    for (auto& overlay : overlays_) {
        if (overlay) {
            overlay->onDetach();
        }
    }
    for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
        if (*it) {
            (*it)->onDetach();
        }
    }
}

void XrLayerStack::onUpdate(const FrameParams& params, LayerParams& out_layers) {
    for (auto& layer : layers_) {
        if (layer && layer->isEnabled()) {
            layer->onUpdate(params, out_layers);
        }
    }
    for (auto& overlay : overlays_) {
        if (overlay && overlay->isEnabled()) {
            overlay->onUpdate(params, out_layers);
        }
    }
}

std::size_t XrLayerStack::layerCount() const {
    return layers_.size() + overlays_.size();
}

}  // namespace vne::xr::samples
