/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 *
 * @file framework_placement_test.cpp
 * @brief Dropping released objects upright onto whatever is below them.
 * ----------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include "interaction/placement.h"

#include <cmath>
#include <vector>

namespace vne::xr::samples {

namespace {

constexpr float kPi = 3.14159265f;
constexpr float kHalf = 0.05f;  // A 10 cm block.

// A table top at 0.82 m, 0.7 x 0.5 m, centered half a meter ahead.
constexpr SupportBox kTable{{0.f, 0.8f, -0.5f}, {0.35f, 0.02f, 0.25f}};

xrmath::Vec3 headingOf(xrmath::Quat q) {
    return xrmath::rotate(q, {0.f, 0.f, -1.f});
}

}  // namespace

TEST(PlacementTest, ABlockReleasedAboveTheTableLandsOnIt) {
    const float height = restingHeight({0.1f, 1.2f, -0.45f}, kHalf, kHalf, {kTable});
    EXPECT_FLOAT_EQ(height, kTable.top());
}

TEST(PlacementTest, ABlockReleasedBesideTheTableFallsToTheFloor) {
    EXPECT_FLOAT_EQ(restingHeight({1.f, 1.2f, -0.45f}, kHalf, kHalf, {kTable}), 0.f);
}

TEST(PlacementTest, ABlockReleasedUnderTheTableStaysUnderIt) {
    EXPECT_FLOAT_EQ(restingHeight({0.f, 0.3f, -0.5f}, kHalf, kHalf, {kTable}), 0.f);
}

TEST(PlacementTest, ABlockReleasedSlightlyInsideTheTableTopStillLandsOnIt) {
    // A hand never lines up exactly: bottom 3 cm below the top.
    EXPECT_FLOAT_EQ(restingHeight({0.f, kTable.top() + kHalf - 0.03f, -0.5f}, kHalf, kHalf, {kTable}), kTable.top());
}

TEST(PlacementTest, BlocksStack) {
    const SupportBox lower{{0.f, kTable.top() + kHalf, -0.5f}, {kHalf, kHalf, kHalf}};

    const float height = restingHeight({0.02f, 1.3f, -0.49f}, kHalf, kHalf, {kTable, lower});

    EXPECT_FLOAT_EQ(height, lower.top());
}

TEST(PlacementTest, ABlockHangingMostlyOffAnEdgeFalls) {
    // Center 6 cm past the table's edge: more than half the block is over empty space.
    EXPECT_FLOAT_EQ(restingHeight({0.41f, 1.2f, -0.5f}, kHalf, kHalf, {kTable}), 0.f);
}

TEST(PlacementTest, UprightYawKeepsTheHeadingAndDropsTheTilt) {
    const xrmath::Quat heading = xrmath::quatFromAxisAngle({0.f, 1.f, 0.f}, 0.6f);
    const xrmath::Quat tilted =
        heading * xrmath::quatFromAxisAngle({1.f, 0.f, 0.f}, 0.4f) * xrmath::quatFromAxisAngle({0.f, 0.f, 1.f}, -0.3f);

    const xrmath::Quat upright = uprightYaw(tilted);

    const xrmath::Vec3 up = xrmath::rotate(upright, {0.f, 1.f, 0.f});
    EXPECT_NEAR(up.y, 1.f, 1e-4f) << "No tilt left";
    const xrmath::Vec3 expected = headingOf(heading);
    const xrmath::Vec3 actual = headingOf(upright);
    EXPECT_NEAR(actual.x, expected.x, 0.05f);
    EXPECT_NEAR(actual.z, expected.z, 0.05f);
}

TEST(PlacementTest, UprightYawHandlesAnObjectPointingStraightUp) {
    const xrmath::Quat pointing_up = xrmath::quatFromAxisAngle({1.f, 0.f, 0.f}, kPi * 0.5f);

    const xrmath::Vec3 up = xrmath::rotate(uprightYaw(pointing_up), {0.f, 1.f, 0.f});

    EXPECT_NEAR(up.y, 1.f, 1e-4f);
    EXPECT_FALSE(std::isnan(up.x));
}

}  // namespace vne::xr::samples
