/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   June 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

#include "stereo_cube_scene.h"

#include <array>
#include <algorithm>
#include <cmath>

namespace vne::xr::examples {

namespace {

constexpr std::array<std::array<float, 3>, 6> kPalette = {{
    {1.f, 0.2f, 0.2f},
    {0.2f, 1.f, 0.2f},
    {0.2f, 0.4f, 1.f},
    {1.f, 0.8f, 0.2f},
    {0.8f, 0.2f, 1.f},
    {0.2f, 1.f, 1.f},
}};

}  // namespace

void StereoCubeScene::reset() {
    blocks_.clear();
    Block table{};
    table.pose.position[1] = 0.8f;
    table.color[0] = 0.35f;
    table.color[1] = 0.35f;
    table.color[2] = 0.4f;
    blocks_.push_back(table);

    for (int i = 0; i < 3; ++i) {
        Block block{};
        block.pose.position[0] = static_cast<float>(i - 1) * 0.15f;
        block.pose.position[1] = 1.0f;
        block.pose.position[2] = -0.4f;
        block.color[0] = kPalette[static_cast<std::size_t>(i)][0];
        block.color[1] = kPalette[static_cast<std::size_t>(i)][1];
        block.color[2] = kPalette[static_cast<std::size_t>(i)][2];
        blocks_.push_back(block);
    }
    next_color_index_ = 3;
}

void StereoCubeScene::update(const InputState& input, float delta_seconds) {
    (void)delta_seconds;
    const std::uint32_t count = std::min(input.controller_count, InputState::kMaxControllers);
    for (std::uint32_t hand = 0; hand < count; ++hand) {
        const bool spawn_now = input.select_pressed[hand];
        if (spawn_now && !prev_spawn_pressed_[hand] && input.pose_valid[hand]) {
            Block block{};
            block.pose = input.controller_poses[hand];
            block.pose.position[2] -= 0.1f;
            const auto& color = kPalette[next_color_index_ % kPalette.size()];
            block.color[0] = color[0];
            block.color[1] = color[1];
            block.color[2] = color[2];
            blocks_.push_back(block);
            ++next_color_index_;
        }
        prev_spawn_pressed_[hand] = spawn_now;

        const bool grab_now = input.squeeze_pressed[hand];
        if (grab_now && !prev_grab_pressed_[hand]) {
            float best_dist = 0.25f;
            Block* best = nullptr;
            for (auto& block : blocks_) {
                if (block.grabbed) {
                    continue;
                }
                const float dx = block.pose.position[0] - input.controller_poses[hand].position[0];
                const float dy = block.pose.position[1] - input.controller_poses[hand].position[1];
                const float dz = block.pose.position[2] - input.controller_poses[hand].position[2];
                const float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
                if (dist < best_dist) {
                    best_dist = dist;
                    best = &block;
                }
            }
            if (best != nullptr) {
                best->grabbed = true;
            }
        }
        if (!grab_now) {
            for (auto& block : blocks_) {
                if (block.grabbed) {
                    block.grabbed = false;
                }
            }
        }
        prev_grab_pressed_[hand] = grab_now;

        if (input.change_color_pressed[hand]) {
            for (auto& block : blocks_) {
                if (block.grabbed) {
                    const auto& color = kPalette[next_color_index_ % kPalette.size()];
                    block.color[0] = color[0];
                    block.color[1] = color[1];
                    block.color[2] = color[2];
                    ++next_color_index_;
                    break;
                }
            }
        }

        for (auto& block : blocks_) {
            if (block.grabbed && input.pose_valid[hand]) {
                block.pose = input.controller_poses[hand];
                block.pose.position[2] -= 0.05f;
            }
        }
    }
}

}  // namespace vne::xr::examples
