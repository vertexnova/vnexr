#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "common/xr_math.h"

#include "vertexnova/xr/input.h"

namespace vne::xr::samples {

/**
 * @brief Moves, turns and zooms a whole scene with the grip buttons, for every sample alike.
 *
 * @details The navigator owns `world_from_scene`: the transform from a sample's own coordinates to the headset's
 * reference space. One grip held makes the scene follow that hand rigidly (grab the world). Both grips held scale
 * the scene by the change in hand distance and turn it about the vertical axis, around the midpoint of the hands.
 * Samples never see this: the host draws them with `world_from_scene` and hands them input already expressed in
 * scene coordinates (`toSceneSpace()`).
 */
class SceneNavigator {
   public:
    static constexpr float kMinScale = 0.2f;
    static constexpr float kMaxScale = 8.f;
    /** A grip held past the press threshold stays held until its value falls below this. */
    static constexpr float kGripReleaseValue = 0.35f;
    /**
     * Height of the eyes above the scene origin after a reset. Samples author content for a seated user: a table at
     * 0.8 m ends up about 45 cm below the eyes, whatever the reference space calls "floor".
     */
    static constexpr float kSceneEyeHeight = 1.25f;

    /**
     * @brief Put the scene origin `kSceneEyeHeight` below @p head, directly under it, facing the way the head looks,
     * at scale 1. Content authored about 0.5 m ahead (-Z) then appears in front of the user.
     */
    void reset(const Pose& head);

    /**
     * @brief Turn the whole scene about the vertical axis through @p head by @p radians (positive turns
     * counter-clockwise seen from above, which looks like the user turning right).
     */
    void snapTurn(const Pose& head, float radians);

    /** @brief Follow the hands. Call once per frame with the unmodified (world space) input. */
    void update(const InputState& input);

    [[nodiscard]] const xrmath::Similarity& worldFromScene() const { return world_from_scene_; }
    [[nodiscard]] xrmath::Mat4 worldFromSceneMatrix() const { return xrmath::toMat4(world_from_scene_); }
    [[nodiscard]] float scale() const { return world_from_scene_.s; }
    [[nodiscard]] bool isGrabbing() const { return mode_ != Mode::eNone; }
    /** @brief 0 when idle, 1 for a one-handed drag, 2 for a two-handed zoom. Changes when the grab changes. */
    [[nodiscard]] int gripCount() const { return mode_ == Mode::eNone ? 0 : (mode_ == Mode::eSingle ? 1 : 2); }

    /** @brief @p input with controller and aim poses converted from world to scene coordinates. */
    [[nodiscard]] InputState toSceneSpace(const InputState& input) const;

   private:
    enum class Mode { eNone, eSingle, eDual };

    xrmath::Similarity world_from_scene_{};
    bool is_grip_down_[2] = {false, false};
    Mode mode_ = Mode::eNone;
    int anchor_hand_ = 0;
    /** eSingle: where the hand and the scene were when the grip was pressed. Only the position follows the hand. */
    xrmath::Vec3 start_hand_position_{};
    /** eSingle and eDual: the scene when the current grab started. */
    xrmath::Similarity start_scene_{};
    xrmath::Vec3 start_midpoint_{};
    xrmath::Vec3 start_direction_{};
    float start_distance_ = 1.f;
};

}  // namespace vne::xr::samples
