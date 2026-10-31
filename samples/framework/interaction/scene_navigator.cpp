/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "interaction/scene_navigator.h"

#include <algorithm>
#include <cmath>

namespace vne::xr::samples {

namespace {

using xrmath::Quat;
using xrmath::Similarity;
using xrmath::Vec3;

constexpr float kMinHandDistance = 0.02f;

Vec3 flattened(Vec3 v) {
    return {v.x, 0.f, v.z};
}

}  // namespace

void SceneNavigator::reset(const Pose& head) {
    const Vec3 head_position = xrmath::positionOf(head);
    Vec3 forward = flattened(xrmath::rotate(xrmath::normalized(xrmath::orientationOf(head)), Vec3{0.f, 0.f, -1.f}));
    if (xrmath::length(forward) < 1e-3f) {
        // Looking straight up or down: fall back to the head's up axis as the heading.
        forward = flattened(xrmath::rotate(xrmath::normalized(xrmath::orientationOf(head)), Vec3{0.f, 1.f, 0.f}));
    }
    forward = xrmath::normalized(forward);
    if (xrmath::length(forward) < 0.5f) {
        forward = Vec3{0.f, 0.f, -1.f};
    }
    // Yaw that turns the scene's -Z onto the heading.
    const float yaw = std::atan2(-forward.x, -forward.z);

    world_from_scene_.t = {head_position.x, head_position.y - kSceneEyeHeight, head_position.z};
    world_from_scene_.q = xrmath::quatFromAxisAngle({0.f, 1.f, 0.f}, yaw);
    world_from_scene_.s = 1.f;
    mode_ = Mode::eNone;
}

void SceneNavigator::snapTurn(const Pose& head, float radians) {
    const Quat turn = xrmath::quatFromAxisAngle({0.f, 1.f, 0.f}, radians);
    const Vec3 pivot = xrmath::positionOf(head);
    world_from_scene_.t = pivot + xrmath::rotate(turn, world_from_scene_.t - pivot);
    world_from_scene_.q = xrmath::normalized(turn * world_from_scene_.q);
    // A turn in the middle of a grab would fight the hands: restart the grab from here.
    mode_ = Mode::eNone;
}

void SceneNavigator::update(const InputState& input) {
    bool is_held[2] = {false, false};
    int held_count = 0;
    for (int hand = 0; hand < 2; ++hand) {
        const auto index = static_cast<std::size_t>(hand);
        // Hysteresis: a grip that wavers around the press threshold must not drop the scene or start a zoom.
        const bool can_grip = input.is_focused && input.pose_valid[index];
        const bool is_pressed = input.squeeze_pressed[index];
        is_grip_down_[index] =
            can_grip && (is_pressed || (is_grip_down_[index] && input.squeeze_value[index] > kGripReleaseValue));
        is_held[hand] = is_grip_down_[index];
        held_count += is_held[hand] ? 1 : 0;
    }

    const Mode wanted = held_count == 2 ? Mode::eDual : (held_count == 1 ? Mode::eSingle : Mode::eNone);
    const int single_hand = is_held[0] ? 0 : 1;
    const bool is_new_grab = wanted != mode_ || (wanted == Mode::eSingle && single_hand != anchor_hand_);

    if (wanted == Mode::eNone) {
        mode_ = Mode::eNone;
        return;
    }

    if (wanted == Mode::eSingle) {
        // Move only: following the hand's rotation as well swings the whole world with every wrist turn.
        const Vec3 hand_position = xrmath::positionOf(input.controller_poses[single_hand]);
        if (is_new_grab) {
            mode_ = Mode::eSingle;
            anchor_hand_ = single_hand;
            start_hand_position_ = hand_position;
            start_scene_ = world_from_scene_;
        }
        world_from_scene_.t = start_scene_.t + (hand_position - start_hand_position_);
        return;
    }

    const Vec3 left = xrmath::positionOf(input.controller_poses[0]);
    const Vec3 right = xrmath::positionOf(input.controller_poses[1]);
    const Vec3 between = right - left;
    const float distance = xrmath::length(between);
    if (distance < kMinHandDistance) {
        return;
    }
    const Vec3 midpoint = (left + right) * 0.5f;
    if (is_new_grab) {
        mode_ = Mode::eDual;
        start_scene_ = world_from_scene_;
        start_midpoint_ = midpoint;
        start_direction_ = between;
        start_distance_ = distance;
    }

    const float wanted_scale = std::clamp(start_scene_.s * distance / start_distance_, kMinScale, kMaxScale);
    const float factor = wanted_scale / start_scene_.s;

    // Turn about the vertical axis by the change in the hands' heading.
    Quat yaw{};
    const Vec3 from = xrmath::normalized(flattened(start_direction_));
    const Vec3 to = xrmath::normalized(flattened(between));
    if (xrmath::length(from) > 0.5f && xrmath::length(to) > 0.5f) {
        yaw = xrmath::quatFromTo(from, to);
    }

    world_from_scene_.q = xrmath::normalized(yaw * start_scene_.q);
    world_from_scene_.s = wanted_scale;
    world_from_scene_.t = midpoint + xrmath::rotate(yaw, (start_scene_.t - start_midpoint_) * factor);
}

InputState SceneNavigator::toSceneSpace(const InputState& input) const {
    InputState out = input;
    const Similarity scene_from_world = xrmath::inverse(world_from_scene_);
    const auto convert = [&scene_from_world](Pose& pose) {
        const Vec3 position = xrmath::apply(scene_from_world, xrmath::positionOf(pose));
        const Quat orientation = scene_from_world.q * xrmath::normalized(xrmath::orientationOf(pose));
        pose = xrmath::makePose(position, orientation);
    };
    for (std::size_t hand = 0; hand < InputState::kMaxControllers; ++hand) {
        if (input.pose_valid[hand]) {
            convert(out.controller_poses[hand]);
        }
        if (input.aim_valid[hand]) {
            convert(out.aim_poses[hand]);
        }
    }
    return out;
}

}  // namespace vne::xr::samples
