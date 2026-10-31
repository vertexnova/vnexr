#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "layers/xr_layer.h"
#include "render/flat_color_renderer.h"

#include "vertexnova/xr/input.h"
#include "vertexnova/xr/types.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace vne::xr::samples {

struct Block {
    Pose pose{};
    float half_extent[3]{0.05f, 0.05f, 0.05f};
    float color[3]{1.f, 0.5f, 0.2f};
    bool is_static = false;
    /** Hand holding the block (0 left, 1 right), or -1. */
    int held_by = -1;
    /** Block pose in the holding hand's frame, captured when it was picked up. */
    Pose offset_in_hand{};
};

/**
 * @brief Blocks on a table: grab with the trigger, spawn by pulling the trigger in empty space, recolor with A/X.
 * A released block turns upright and drops onto the table, another block, or the floor.
 */
class StereoCubeScene {
   public:
    static constexpr std::size_t kMaxBlocks = 150;

    void reset();
    void update(const InputState& input);
    [[nodiscard]] const std::vector<Block>& blocks() const { return blocks_; }
    /** @brief Index of the block a trigger pull would pick up right now, or -1. */
    [[nodiscard]] int hoveredBlock() const { return hovered_block_; }

   private:
    void addBlock(const Pose& pose);
    void tryPickOrSpawn(std::uint32_t hand, const Pose& hand_pose);
    void release(std::uint32_t hand);
    void settle(Block& block) const;
    [[nodiscard]] int pickCandidate(const Pose& hand_pose) const;

    std::vector<Block> blocks_;
    std::uint32_t next_color_index_ = 0;
    int hovered_block_ = -1;
    bool prev_trigger_[InputState::kMaxControllers]{};
    bool prev_color_[InputState::kMaxControllers]{};
};

/** @brief Stereo cube scene for OpenXR: all drawing goes through the framework's per-eye render hook. */
class StereoCubeSceneLayer final : public IXrLayer {
   public:
    StereoCubeSceneLayer();
    ~StereoCubeSceneLayer() override;

    void onAttach(ISession* session) override;
    void onDetach() override;
    void onUpdate(const FrameParams& params, LayerParams& out_layers) override;
    void onRenderEye(const EyeRenderContext& context) override;

   private:
    FlatColorRenderer renderer_;
    StereoCubeScene scene_;
    bool is_ready_ = false;
};

}  // namespace vne::xr::samples
