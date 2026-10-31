/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "stereo_cube_scene_layer.h"

#include "common/xr_math.h"
#include "interaction/placement.h"

#include "vertexnova/logging/logging.h"

#include <algorithm>
#include <array>

namespace vne::xr::samples {

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.xr.samples.stereo_cube");

constexpr std::array<std::array<float, 3>, 6> kPalette = {{
    {1.f, 0.2f, 0.2f},
    {0.2f, 1.f, 0.2f},
    {0.2f, 0.4f, 1.f},
    {1.f, 0.8f, 0.2f},
    {0.8f, 0.2f, 1.f},
    {0.2f, 1.f, 1.f},
}};

constexpr float kBlockHalfExtent = 0.05f;
constexpr float kPickRadius = 0.12f;
constexpr float kSpawnDistance = 0.10f;
constexpr float kTriggerThreshold = 0.5f;

using xrmath::Quat;
using xrmath::Vec3;

void setColor(Block& block, std::uint32_t index) {
    const auto& color = kPalette[index % kPalette.size()];
    block.color[0] = color[0];
    block.color[1] = color[1];
    block.color[2] = color[2];
}

}  // namespace

void StereoCubeScene::reset() {
    blocks_.clear();

    Block floor{};
    floor.is_static = true;
    floor.half_extent[0] = 2.f;
    floor.half_extent[1] = 0.01f;
    floor.half_extent[2] = 2.f;
    floor.pose.position[1] = -0.01f;
    floor.color[0] = 0.10f;
    floor.color[1] = 0.12f;
    floor.color[2] = 0.16f;
    blocks_.push_back(floor);

    Block table{};
    table.is_static = true;
    table.half_extent[0] = 0.35f;
    table.half_extent[1] = 0.02f;
    table.half_extent[2] = 0.25f;
    table.pose.position[1] = 0.8f;
    table.pose.position[2] = -0.5f;
    table.color[0] = 0.45f;
    table.color[1] = 0.45f;
    table.color[2] = 0.5f;
    blocks_.push_back(table);

    for (int i = 0; i < 3; ++i) {
        Block block{};
        block.pose.position[0] = static_cast<float>(i - 1) * 0.15f;
        block.pose.position[1] = 0.8f + 0.02f + kBlockHalfExtent;
        block.pose.position[2] = -0.5f;
        setColor(block, static_cast<std::uint32_t>(i));
        blocks_.push_back(block);
    }
    next_color_index_ = 3;
    std::fill(std::begin(prev_trigger_), std::end(prev_trigger_), false);
    std::fill(std::begin(prev_color_), std::end(prev_color_), false);
}

void StereoCubeScene::addBlock(const Pose& pose) {
    if (blocks_.size() >= kMaxBlocks) {
        // Drop the oldest dynamic block that nobody holds so the draw count stays bounded.
        const auto oldest =
            std::find_if(blocks_.begin(), blocks_.end(), [](const Block& b) { return !b.is_static && b.held_by < 0; });
        if (oldest != blocks_.end()) {
            blocks_.erase(oldest);
        }
    }
    Block block{};
    block.pose = pose;
    setColor(block, next_color_index_++);
    blocks_.push_back(block);
}

void StereoCubeScene::settle(Block& block) const {
    std::vector<SupportBox> supports;
    supports.reserve(blocks_.size());
    for (const Block& other : blocks_) {
        if (&other == &block || other.held_by >= 0) {
            continue;
        }
        supports.push_back(
            {xrmath::positionOf(other.pose), {other.half_extent[0], other.half_extent[1], other.half_extent[2]}});
    }
    const Vec3 position = xrmath::positionOf(block.pose);
    const float support_top =
        restingHeight(position, block.half_extent[1], block.half_extent[0], supports, /*floor_y=*/0.f);
    block.pose = xrmath::makePose({position.x, support_top + block.half_extent[1], position.z},
                                  uprightYaw(xrmath::orientationOf(block.pose)));
}

void StereoCubeScene::release(std::uint32_t hand) {
    for (auto& block : blocks_) {
        if (block.held_by == static_cast<int>(hand)) {
            block.held_by = -1;
            settle(block);
        }
    }
}

int StereoCubeScene::pickCandidate(const Pose& hand_pose) const {
    const Vec3 hand_position = xrmath::positionOf(hand_pose);
    int best = -1;
    float best_distance = kPickRadius;
    for (std::size_t i = 0; i < blocks_.size(); ++i) {
        const Block& block = blocks_[i];
        if (block.is_static || block.held_by >= 0) {
            continue;
        }
        const float distance = xrmath::length(xrmath::positionOf(block.pose) - hand_position);
        if (distance < best_distance) {
            best_distance = distance;
            best = static_cast<int>(i);
        }
    }
    return best;
}

void StereoCubeScene::tryPickOrSpawn(std::uint32_t hand, const Pose& hand_pose) {
    const Vec3 hand_position = xrmath::positionOf(hand_pose);
    const Quat hand_orientation = xrmath::normalized(xrmath::orientationOf(hand_pose));

    const int candidate = pickCandidate(hand_pose);
    Block* best = candidate >= 0 ? &blocks_[static_cast<std::size_t>(candidate)] : nullptr;

    if (best == nullptr) {
        // Empty space: a new block appears in front of the hand and is held right away.
        Pose spawn_pose =
            xrmath::makePose(hand_position + xrmath::rotate(hand_orientation, Vec3{0.f, 0.f, -kSpawnDistance}),
                             hand_orientation);
        addBlock(spawn_pose);
        best = &blocks_.back();
    }

    best->held_by = static_cast<int>(hand);
    const Quat inverse = xrmath::conjugate(hand_orientation);
    best->offset_in_hand = xrmath::makePose(xrmath::rotate(inverse, xrmath::positionOf(best->pose) - hand_position),
                                            inverse * xrmath::normalized(xrmath::orientationOf(best->pose)));
}

void StereoCubeScene::update(const InputState& input) {
    const std::uint32_t count = std::min(input.controller_count, InputState::kMaxControllers);
    for (std::uint32_t hand = 0; hand < count; ++hand) {
        const bool is_trigger =
            input.is_focused && input.pose_valid[hand] && input.select_value[hand] > kTriggerThreshold;
        if (is_trigger && !prev_trigger_[hand]) {
            tryPickOrSpawn(hand, input.controller_poses[hand]);
        } else if (!is_trigger && prev_trigger_[hand]) {
            release(hand);
        }
        prev_trigger_[hand] = is_trigger;

        // A/X changes the color of the block in this hand, once per press.
        const bool is_color = input.is_focused && input.change_color_pressed[hand];
        if (is_color && !prev_color_[hand]) {
            for (auto& block : blocks_) {
                if (block.held_by == static_cast<int>(hand)) {
                    setColor(block, next_color_index_++);
                    break;
                }
            }
        }
        prev_color_[hand] = is_color;

        if (!input.pose_valid[hand]) {
            continue;
        }
        const Vec3 hand_position = xrmath::positionOf(input.controller_poses[hand]);
        const Quat hand_orientation = xrmath::normalized(xrmath::orientationOf(input.controller_poses[hand]));
        for (auto& block : blocks_) {
            if (block.held_by != static_cast<int>(hand)) {
                continue;
            }
            const Vec3 position =
                hand_position + xrmath::rotate(hand_orientation, xrmath::positionOf(block.offset_in_hand));
            block.pose = xrmath::makePose(position, hand_orientation * xrmath::orientationOf(block.offset_in_hand));
        }
    }
    if (!input.is_focused) {
        release(0);
        release(1);
        std::fill(std::begin(prev_trigger_), std::end(prev_trigger_), false);
    }

    // Show which block a trigger pull would pick: the closest one in reach of a free hand.
    hovered_block_ = -1;
    for (std::uint32_t hand = 0; hand < count && hovered_block_ < 0; ++hand) {
        const bool is_holding = std::any_of(blocks_.begin(), blocks_.end(), [hand](const Block& b) {
            return b.held_by == static_cast<int>(hand);
        });
        if (input.is_focused && input.pose_valid[hand] && !is_holding) {
            hovered_block_ = pickCandidate(input.controller_poses[hand]);
        }
    }
}

StereoCubeSceneLayer::StereoCubeSceneLayer()
    : IXrLayer("StereoCubeScene") {}

StereoCubeSceneLayer::~StereoCubeSceneLayer() {
    onDetach();
}

void StereoCubeSceneLayer::onAttach(ISession* session) {
    onDetach();
    scene_.reset();
    if (session == nullptr || !renderer_.initialize(session->graphicsDevice())) {
        VNE_LOG_ERROR << "Stereo cube: no graphics device; nothing will be drawn";
        return;
    }
    is_ready_ = true;
    VNE_LOG_INFO << "Stereo cube sample ready";
}

void StereoCubeSceneLayer::onDetach() {
    renderer_.shutdown();
    is_ready_ = false;
}

void StereoCubeSceneLayer::onUpdate(const FrameParams& params, LayerParams& out_layers) {
    if (!params.frame.should_render) {
        return;
    }
    scene_.update(params.input);
    // One background for both eyes: a difference between the eyes reads as flicker.
    out_layers.clear_color[0] = 0.03f;
    out_layers.clear_color[1] = 0.04f;
    out_layers.clear_color[2] = 0.07f;
}

void StereoCubeSceneLayer::onRenderEye(const EyeRenderContext& context) {
    if (!is_ready_ || !renderer_.prepare(context)) {
        return;
    }
    const auto& blocks = scene_.blocks();
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        const Block& block = blocks[i];
        // The block a trigger pull would pick glows a little.
        const float glow = static_cast<int>(i) == scene_.hoveredBlock() ? 0.35f : 0.f;
        const float color[4] = {block.color[0] + (1.f - block.color[0]) * glow,
                                block.color[1] + (1.f - block.color[1]) * glow,
                                block.color[2] + (1.f - block.color[2]) * glow,
                                1.f};
        renderer_.drawBox(context,
                          xrmath::fromPose(block.pose),
                          {block.half_extent[0], block.half_extent[1], block.half_extent[2]},
                          color);
    }
}

}  // namespace vne::xr::samples
