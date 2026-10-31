#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "interaction/input_edges.h"
#include "layers/xr_layer.h"
#include "render/flat_color_renderer.h"
#include "render/text_renderer.h"

#include <functional>

namespace vne::xr::samples {

/**
 * @brief Draws a simple box at each hand and a pointing ray, so the user can see where their hands are.
 *
 * @details Hidden while the app has no input focus (system menu open) or a hand is not tracked. Optionally shows a
 * small "MENU" tag above the left controller: pointing the right ray at it and pulling the trigger calls the menu
 * callback. It is a second way back to the launcher that does not depend on the Menu button.
 */
class ControllerVisualsLayer final : public IXrLayer {
   public:
    /** @param text Shared text renderer owned by the host; it must outlive the layer (null: no MENU label). */
    explicit ControllerVisualsLayer(TextRenderer* text = nullptr);
    ~ControllerVisualsLayer() override;

    /** @brief Show or hide the aim ray; the sample menu wants it, a sample may not. */
    void setRayVisible(bool is_visible) { is_ray_visible_ = is_visible; }
    void setMenuTagVisible(bool is_visible) { is_menu_tag_visible_ = is_visible; }
    void setMenuCallback(std::function<void()> callback) { on_menu_ = std::move(callback); }

    void onAttach(ISession* session) override;
    void onDetach() override;
    void onUpdate(const FrameParams& params, LayerParams& out_layers) override;
    void onRenderEye(const EyeRenderContext& context) override;

   private:
    /** Where the tag sits on the left controller, or nothing when it is hidden or the controller is not tracked. */
    [[nodiscard]] bool menuTagPose(xrmath::Similarity& out_pose) const;

    FlatColorRenderer renderer_;
    TextRenderer* text_ = nullptr;
    TextMesh menu_text_;
    InputState input_{};
    Pose head_pose_{};
    bool is_head_pose_valid_ = false;
    InputEdges edges_;
    std::function<void()> on_menu_;
    bool is_ready_ = false;
    bool is_ray_visible_ = true;
    bool is_menu_tag_visible_ = false;
    bool is_menu_hovered_ = false;
    float right_ray_length_ = 0.f;
};

}  // namespace vne::xr::samples
