/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "stereo_cube_scene_layer.h"

#include <algorithm>
#include <cmath>

#include "vertexnova/logging/logging.h"

#ifdef VNE_XR_WITH_RHI
#include "vertexnova/rhi/device.h"
#include "vertexnova/rhi/graphics_factory.h"
#endif

namespace vne::xr::samples {

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
                block.grabbed = false;
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

StereoCubeSceneLayer::StereoCubeSceneLayer()
    : IXrLayer("StereoCubeScene") {}

void StereoCubeSceneLayer::onAttach() {
    scene_.reset();
#ifdef VNE_XR_WITH_RHI
    const auto backend = vne::rhi::GraphicsFactory::getDefaultBackend();
    if (auto physical = vne::rhi::GraphicsFactory::createPhysicalDevice(backend)) {
        vne::rhi::PhysicalDeviceDescriptor phys_desc{};
        const auto info = physical->getDefaultDevice(phys_desc);
        vne::rhi::DeviceDescriptor dev_desc{};
        dev_desc.debug_name = "openxr_stereo_sample";
        device_ = physical->createDevice(info, dev_desc);
    }
#endif
    VNE_LOG_INFO << "OpenXR stereo sample ready";
}

void StereoCubeSceneLayer::clearEyeSurface(const ViewSurface& surface, float r, float g, float b) {
#ifdef VNE_XR_WITH_RHI
    if (!device_ || !surface.color_texture) {
        return;
    }
    VNE_LOG_INFO << "  eye " << surface.view_index << " " << surface.width << "x" << surface.height << " clear=(" << r
                 << "," << g << "," << b << ")";
#else
    (void)surface;
    (void)r;
    (void)g;
    (void)b;
#endif
}

void StereoCubeSceneLayer::onUpdate(const FrameParams& params, LayerParams& out_layers) {
    if (!params.frame.should_render) {
        return;
    }

    scene_.update(params.input, 1.f / 72.f);
    const auto& blocks = scene_.blocks();

    float clear_r = 0.05f;
    float clear_g = 0.05f;
    float clear_b = 0.08f;
    if (blocks.size() > 1) {
        clear_r = blocks[1].color[0] * 0.2f;
        clear_g = blocks[1].color[1] * 0.2f;
        clear_b = blocks[1].color[2] * 0.2f;
    }

    for (std::uint32_t i = 0; i < params.frame.surfaces.view_count; ++i) {
        const float eye_tint = (i == 0) ? 0.02f : -0.02f;
        clearEyeSurface(params.frame.surfaces.views[i], clear_r + eye_tint, clear_g, clear_b + eye_tint);
    }

    ++frames_rendered_;
    if (frames_rendered_ >= 3600) {
        out_layers.request_exit = true;
    }
}

}  // namespace vne::xr::samples
