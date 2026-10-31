#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "interaction/input_edges.h"
#include "launcher/launcher_layout.h"
#include "layers/xr_layer.h"
#include "render/flat_color_renderer.h"
#include "render/text_renderer.h"

#include <array>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace vne::xr::samples {

/** @brief One selectable card on the launcher panel. */
struct LauncherEntry {
    std::string id;  //!< Passed to the select callback; empty for the exit card.
    std::string title;
    std::string description;
    std::array<float, 3> accent{0.3f, 0.6f, 1.f};
    bool is_exit = false;
};

/**
 * @brief Floating panel of sample cards, pointed at with a controller ray (trigger selects) or a hand pinch.
 *
 * @details The panel is placed in front of the head by `show()` and stays fixed in the room, so the user can look
 * around it. It lives in world space: the scene navigator does not move it.
 */
class LauncherLayer final : public IXrLayer {
   public:
    using SelectCallback = std::function<void(const LauncherEntry& entry)>;

    /** @param text Shared text renderer owned by the host; it must outlive the layer. */
    LauncherLayer(std::vector<LauncherEntry> entries, SelectCallback on_select, TextRenderer* text);
    ~LauncherLayer() override;

    /** @brief Show the panel about 1.3 m ahead of @p head, facing it. */
    void show(const Pose& head);
    void hide();
    [[nodiscard]] bool isVisible() const { return is_visible_; }
    /** @brief One line shown under the cards (for example why the last sample failed). Empty clears it. */
    void setStatus(const std::string& status);

    void onAttach(ISession* session) override;
    void onDetach() override;
    void onUpdate(const FrameParams& params, LayerParams& out_layers) override;
    void onRenderEye(const EyeRenderContext& context) override;

   private:
    struct CardMeshes {
        TextMesh title;
        std::vector<TextMesh> description_lines;
    };

    void buildMeshes();
    void destroyMeshes();

    std::vector<LauncherEntry> entries_;
    SelectCallback on_select_;
    LauncherLayout layout_;
    FlatColorRenderer renderer_;
    TextRenderer* text_ = nullptr;
    std::vector<CardMeshes> card_meshes_;
    TextMesh heading_;
    TextMesh footer_;
    TextMesh status_mesh_;
    std::string status_;
    bool is_status_dirty_ = false;
    bool is_ready_ = false;

    bool is_visible_ = false;
    xrmath::Similarity panel_pose_{};
    InputEdges edges_;
    std::optional<std::size_t> hover_card_;
    std::optional<PanelHit> hover_hit_;
    /** Frames since show(): a trigger pull that opened the panel must not also pick a card on it. */
    std::uint32_t frames_visible_ = 0;
};

}  // namespace vne::xr::samples
