#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "layers/xr_layer.h"
#include "render/flat_color_renderer.h"
#include "render/text_renderer.h"

#include "vertexnova/xr/input.h"

#include <array>

namespace vne::xr::samples {

/**
 * @brief Reference scene to run first after any change.
 *
 * @details Shows a 0.5 m floor grid, colored world axes (X red, Y green, Z blue), a 1 m cube two meters ahead (for
 * checking scale and eye separation: it must look single and solid with either eye closed), one light per button
 * above each controller, a strip of recent frame times (green = on time, red = late), and the greeting
 * "Hello VertexNova!" in gray and orange (a check that text is sharp).
 */
class XrDiagnosticsLayer final : public IXrLayer {
   public:
    /** @param text Shared text renderer owned by the host; it must outlive the layer (null: no greeting). */
    explicit XrDiagnosticsLayer(TextRenderer* text);
    ~XrDiagnosticsLayer() override;

    void onAttach(ISession* session) override;
    void onDetach() override;
    void onUpdate(const FrameParams& params, LayerParams& out_layers) override;
    void onRenderEye(const EyeRenderContext& context) override;

   private:
    static constexpr std::size_t kHistory = 60;
    static constexpr std::size_t kIndicators = 6;

    void recordFrameTime(double predicted_display_time);
    void drawIndicators(const EyeRenderContext& context, std::size_t hand) const;

    FlatColorRenderer renderer_;
    TextRenderer* text_ = nullptr;
    TextMesh hello_;
    TextMesh brand_;
    InputState input_{};
    bool is_ready_ = false;

    double last_display_time_ = 0.0;
    std::array<float, kHistory> frame_ms_{};
    std::array<bool, kHistory> is_late_{};
    std::size_t history_count_ = 0;
    std::size_t history_next_ = 0;
    float median_ms_ = 0.f;
};

}  // namespace vne::xr::samples
