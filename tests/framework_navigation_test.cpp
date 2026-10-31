/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 *
 * @file framework_navigation_test.cpp
 * @brief Scene navigation and button edge detection of the sample framework.
 * ----------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include "common/xr_math.h"
#include "interaction/input_edges.h"
#include "interaction/scene_navigator.h"

#include <cmath>

namespace vne::xr::samples {

namespace {

using xrmath::Quat;
using xrmath::Vec3;

constexpr float kPi = 3.14159265f;

Pose headAt(Vec3 position, float yaw_radians) {
    return xrmath::makePose(position, xrmath::quatFromAxisAngle({0.f, 1.f, 0.f}, yaw_radians));
}

InputState gripsAt(Vec3 left, Vec3 right, bool is_left_held, bool is_right_held) {
    InputState input;
    input.controller_count = 2;
    input.pose_valid[0] = input.pose_valid[1] = true;
    input.controller_poses[0] = xrmath::makePose(left, Quat{});
    input.controller_poses[1] = xrmath::makePose(right, Quat{});
    input.squeeze_pressed[0] = is_left_held;
    input.squeeze_pressed[1] = is_right_held;
    return input;
}

void expectNear(Vec3 actual, Vec3 expected, float tolerance = 1e-3f) {
    EXPECT_NEAR(actual.x, expected.x, tolerance);
    EXPECT_NEAR(actual.y, expected.y, tolerance);
    EXPECT_NEAR(actual.z, expected.z, tolerance);
}

}  // namespace

TEST(SceneNavigatorTest, ResetPlacesTheSceneAheadOfTheHeadAtSeatedHeight) {
    SceneNavigator navigator;
    // Head 1.6 m high at (1, 2), turned to look along -X.
    navigator.reset(headAt({1.f, 1.6f, 2.f}, kPi * 0.5f));

    // Content authored one meter ahead (-Z) must end up one meter ahead of the head.
    const float origin_y = 1.6f - SceneNavigator::kSceneEyeHeight;
    expectNear(xrmath::apply(navigator.worldFromScene(), {0.f, 0.f, -1.f}), {0.f, origin_y, 2.f});
    // The scene's eye height lands exactly at the head.
    expectNear(xrmath::apply(navigator.worldFromScene(), {0.f, SceneNavigator::kSceneEyeHeight, 0.f}),
               {1.f, 1.6f, 2.f});
    EXPECT_FLOAT_EQ(navigator.scale(), 1.f);
}

TEST(SceneNavigatorTest, ResetIsRelativeToTheHeadWhateverTheSpaceCallsTheFloor) {
    SceneNavigator sitting;
    sitting.reset(headAt({0.f, 1.1f, 0.f}, 0.f));
    SceneNavigator head_origin_space;
    head_origin_space.reset(headAt({0.f, 0.f, 0.f}, 0.f));

    EXPECT_NEAR(sitting.worldFromScene().t.y, 1.1f - SceneNavigator::kSceneEyeHeight, 1e-5f);
    EXPECT_NEAR(head_origin_space.worldFromScene().t.y, -SceneNavigator::kSceneEyeHeight, 1e-5f);
}

TEST(SceneNavigatorTest, OneGripMovesWithoutTurningOrTilting) {
    SceneNavigator navigator;
    navigator.reset(headAt({0.f, 1.6f, 0.f}, 0.f));
    const Quat before = navigator.worldFromScene().q;
    const Vec3 origin_before = navigator.worldFromScene().t;

    InputState input = gripsAt({0.f, 1.f, -0.4f}, {0.3f, 1.f, -0.4f}, false, true);
    navigator.update(input);
    // Wrist rolled 90 degrees and the hand moved 10 cm: only the move may reach the scene.
    input.controller_poses[1] =
        xrmath::makePose({0.4f, 1.f, -0.4f}, xrmath::quatFromAxisAngle({0.f, 0.f, 1.f}, kPi * 0.5f));
    navigator.update(input);

    const Quat after = navigator.worldFromScene().q;
    EXPECT_NEAR(std::abs(before.x * after.x + before.y * after.y + before.z * after.z + before.w * after.w),
                1.f,
                1e-5f);
    expectNear(navigator.worldFromScene().t, origin_before + Vec3{0.1f, 0.f, 0.f});
}

TEST(SceneNavigatorTest, SnapTurnSwingsTheSceneAroundTheHead) {
    SceneNavigator navigator;
    const Pose head = headAt({0.5f, 1.6f, 0.5f}, 0.f);
    navigator.reset(head);
    const Vec3 ahead = xrmath::apply(navigator.worldFromScene(), {0.f, SceneNavigator::kSceneEyeHeight, -1.f});

    navigator.snapTurn(head, kPi * 0.5f);  // A right flick: what was on the right comes to the front.

    const Vec3 turned = xrmath::apply(navigator.worldFromScene(), {0.f, SceneNavigator::kSceneEyeHeight, -1.f});
    EXPECT_NEAR(xrmath::length(turned - xrmath::positionOf(head)),
                xrmath::length(ahead - xrmath::positionOf(head)),
                1e-4f);
    // Content that was ahead is now on the left.
    expectNear(turned, {-0.5f, 1.6f, 0.5f});
}

TEST(SceneNavigatorTest, OneGripDragsTheScene) {
    SceneNavigator navigator;
    navigator.reset(headAt({0.f, 1.6f, 0.f}, 0.f));
    const Vec3 origin_before = navigator.worldFromScene().t;

    navigator.update(gripsAt({-0.3f, 1.f, -0.4f}, {0.3f, 1.f, -0.4f}, false, true));
    navigator.update(gripsAt({-0.3f, 1.f, -0.4f}, {0.8f, 1.2f, -0.4f}, false, true));

    EXPECT_TRUE(navigator.isGrabbing());
    expectNear(navigator.worldFromScene().t, origin_before + Vec3{0.5f, 0.2f, 0.f});
    EXPECT_FLOAT_EQ(navigator.scale(), 1.f);
}

TEST(SceneNavigatorTest, ReleasingTheGripKeepsTheScenePut) {
    SceneNavigator navigator;
    navigator.reset(headAt({0.f, 1.6f, 0.f}, 0.f));
    navigator.update(gripsAt({0.f, 1.f, -0.4f}, {0.3f, 1.f, -0.4f}, false, true));
    navigator.update(gripsAt({0.f, 1.f, -0.4f}, {0.6f, 1.f, -0.4f}, false, true));
    const Vec3 held = navigator.worldFromScene().t;

    navigator.update(gripsAt({0.f, 1.f, -0.4f}, {2.f, 1.f, -0.4f}, false, false));

    EXPECT_FALSE(navigator.isGrabbing());
    expectNear(navigator.worldFromScene().t, held);
}

TEST(SceneNavigatorTest, TwoGripsZoomAboutTheMidpoint) {
    SceneNavigator navigator;
    navigator.reset(headAt({0.f, 1.6f, 0.f}, 0.f));
    const Vec3 midpoint{0.f, 1.2f, -0.5f};

    navigator.update(gripsAt({-0.2f, 1.2f, -0.5f}, {0.2f, 1.2f, -0.5f}, true, true));
    // The scene point under the midpoint at the start of the grab must stay under the midpoint while zooming.
    const Vec3 anchored_scene_point = xrmath::apply(xrmath::inverse(navigator.worldFromScene()), midpoint);
    navigator.update(gripsAt({-0.4f, 1.2f, -0.5f}, {0.4f, 1.2f, -0.5f}, true, true));

    EXPECT_NEAR(navigator.scale(), 2.f, 1e-3f);
    expectNear(xrmath::apply(navigator.worldFromScene(), anchored_scene_point), midpoint);
}

TEST(SceneNavigatorTest, ZoomIsClamped) {
    SceneNavigator navigator;
    navigator.reset(headAt({0.f, 1.6f, 0.f}, 0.f));
    navigator.update(gripsAt({-0.01f, 1.f, -0.5f}, {0.01f, 1.f, -0.5f}, true, true));  // Too close: ignored.
    navigator.update(gripsAt({-0.1f, 1.f, -0.5f}, {0.1f, 1.f, -0.5f}, true, true));
    navigator.update(gripsAt({-50.f, 1.f, -0.5f}, {50.f, 1.f, -0.5f}, true, true));
    EXPECT_FLOAT_EQ(navigator.scale(), SceneNavigator::kMaxScale);

    navigator.update(gripsAt({-0.0001f, 1.f, -0.5f}, {0.0001f, 1.f, -0.5f}, true, true));
    EXPECT_GE(navigator.scale(), SceneNavigator::kMinScale);
}

TEST(SceneNavigatorTest, TwoGripsTurnTheSceneByTheHandsHeading) {
    SceneNavigator navigator;
    navigator.reset(headAt({0.f, 1.6f, 0.f}, 0.f));
    navigator.update(gripsAt({-0.2f, 1.f, -0.5f}, {0.2f, 1.f, -0.5f}, true, true));
    const Quat before = navigator.worldFromScene().q;

    // Right hand swings back by 0.4 m while the left swings forward: the line between the hands turns 90 degrees.
    navigator.update(gripsAt({0.f, 1.f, -0.7f}, {0.f, 1.f, -0.3f}, true, true));

    const Vec3 x_before = xrmath::rotate(before, {1.f, 0.f, 0.f});
    const Vec3 x_after = xrmath::rotate(navigator.worldFromScene().q, {1.f, 0.f, 0.f});
    EXPECT_NEAR(xrmath::dot(x_before, x_after), 0.f, 1e-2f) << "A quarter turn";
    EXPECT_NEAR(xrmath::length(x_after), 1.f, 1e-3f);
}

TEST(SceneNavigatorTest, ToSceneSpaceUndoesTheNavigation) {
    SceneNavigator navigator;
    navigator.reset(headAt({0.f, 1.6f, 0.f}, kPi * 0.5f));
    navigator.update(gripsAt({-0.2f, 1.2f, -0.5f}, {0.2f, 1.2f, -0.5f}, true, true));
    navigator.update(gripsAt({-0.4f, 1.2f, -0.5f}, {0.4f, 1.2f, -0.5f}, true, true));

    InputState world = gripsAt({0.1f, 1.3f, -0.6f}, {0.5f, 1.1f, -0.2f}, false, false);
    world.aim_valid[1] = true;
    world.aim_poses[1] = world.controller_poses[1];
    const InputState scene = navigator.toSceneSpace(world);

    const Vec3 round_trip = xrmath::apply(navigator.worldFromScene(), xrmath::positionOf(scene.controller_poses[1]));
    expectNear(round_trip, {0.5f, 1.1f, -0.2f});
    const Vec3 aim_round_trip = xrmath::apply(navigator.worldFromScene(), xrmath::positionOf(scene.aim_poses[1]));
    expectNear(aim_round_trip, {0.5f, 1.1f, -0.2f});
}

TEST(SceneNavigatorTest, LosingFocusReleasesTheGrab) {
    SceneNavigator navigator;
    navigator.reset(headAt({0.f, 1.6f, 0.f}, 0.f));
    navigator.update(gripsAt({0.f, 1.f, -0.4f}, {0.3f, 1.f, -0.4f}, false, true));
    EXPECT_TRUE(navigator.isGrabbing());

    InputState unfocused = gripsAt({0.f, 1.f, -0.4f}, {0.3f, 1.f, -0.4f}, false, true);
    unfocused.is_focused = false;
    navigator.update(unfocused);

    EXPECT_FALSE(navigator.isGrabbing());
}

TEST(InputEdgesTest, ReportsEachTransitionForExactlyOneFrame) {
    InputEdges edges;
    InputState input;
    input.select_value[1] = 0.9f;

    edges.update(input);
    EXPECT_TRUE(edges.triggerPressed(1));
    EXPECT_TRUE(edges.triggerDown(1));
    EXPECT_FALSE(edges.triggerPressed(0));

    edges.update(input);
    EXPECT_FALSE(edges.triggerPressed(1)) << "Held, not pressed again";
    EXPECT_TRUE(edges.triggerDown(1));

    input.select_value[1] = 0.1f;
    edges.update(input);
    EXPECT_TRUE(edges.triggerReleased(1));
    EXPECT_FALSE(edges.triggerDown(1));
}

TEST(InputEdgesTest, AThumbstickFlickFiresOnceUntilTheStickReturnsToCentre) {
    InputEdges edges;
    InputState input;
    input.thumbstick[1][0] = 0.9f;

    edges.update(input);
    EXPECT_TRUE(edges.stickFlickedRight(1));
    edges.update(input);
    EXPECT_FALSE(edges.stickFlickedRight(1)) << "Holding the stick does not repeat the turn";

    input.thumbstick[1][0] = 0.5f;  // Between the thresholds: still not re-armed.
    edges.update(input);
    input.thumbstick[1][0] = 0.9f;
    edges.update(input);
    EXPECT_FALSE(edges.stickFlickedRight(1));

    input.thumbstick[1][0] = 0.f;
    edges.update(input);
    input.thumbstick[1][0] = -0.95f;
    edges.update(input);
    EXPECT_TRUE(edges.stickFlickedLeft(1));
    EXPECT_FALSE(edges.stickFlickedLeft(0));
}

TEST(InputEdgesTest, PinchCountsAsATriggerPull) {
    InputEdges edges;
    InputState input;
    input.select_pressed[0] = true;

    edges.update(input);

    EXPECT_TRUE(edges.triggerPressed(0));
}

TEST(InputEdgesTest, LosingFocusReleasesEverythingAndRegainingItDoesNotRepeatAPress) {
    InputEdges edges;
    InputState input;
    input.menu_pressed = true;
    input.secondary_pressed[0] = true;
    edges.update(input);
    EXPECT_TRUE(edges.menuPressed());
    EXPECT_TRUE(edges.secondaryPressed(0));

    input.is_focused = false;
    edges.update(input);
    EXPECT_FALSE(edges.menuPressed());
    EXPECT_FALSE(edges.secondaryPressed(0));

    // Still physically held when focus returns: that is a fresh press, and only one.
    input.is_focused = true;
    edges.update(input);
    EXPECT_TRUE(edges.menuPressed());
    edges.update(input);
    EXPECT_FALSE(edges.menuPressed());
}

}  // namespace vne::xr::samples
