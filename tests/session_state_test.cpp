/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   October 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 *
 * @file session_state_test.cpp
 * @brief Session lifecycle rules shared by every backend.
 * ----------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include "vertexnova/xr/core/session_state.h"

namespace vne::xr {

TEST(SessionStateTest, ReadyBeginsTheSessionOnlyOnce) {
    const auto first = evaluateSessionTransition(SessionState::eReady, false);
    const auto repeated = evaluateSessionTransition(SessionState::eReady, true);

    EXPECT_TRUE(first.should_begin_session);
    EXPECT_FALSE(first.is_frame_loop_active) << "The loop starts only after xrBeginSession succeeds";
    EXPECT_FALSE(repeated.should_begin_session);
    EXPECT_TRUE(repeated.is_frame_loop_active);
}

TEST(SessionStateTest, RunningStatesActivateTheFrameLoop) {
    for (const auto state : {SessionState::eSynchronized, SessionState::eVisible, SessionState::eFocused}) {
        const auto transition = evaluateSessionTransition(state, true);

        EXPECT_TRUE(transition.is_frame_loop_active);
        EXPECT_FALSE(transition.should_leave_loop);
        EXPECT_EQ(transition.state, state);
    }
}

TEST(SessionStateTest, StoppingEndsTheSessionAndReleasesSwapchains) {
    const auto transition = evaluateSessionTransition(SessionState::eStopping, true);

    EXPECT_TRUE(transition.should_end_session);
    EXPECT_TRUE(transition.should_release_swapchains);
    EXPECT_FALSE(transition.is_frame_loop_active);
    EXPECT_FALSE(transition.should_leave_loop) << "STOPPING is followed by IDLE or EXITING, so the loop keeps polling";
}

TEST(SessionStateTest, StoppingBeforeTheSessionWasBegunDoesNotEndIt) {
    EXPECT_FALSE(evaluateSessionTransition(SessionState::eStopping, false).should_end_session);
}

TEST(SessionStateTest, LossPendingAndExitingLeaveTheLoop) {
    for (const auto state : {SessionState::eLossPending, SessionState::eExiting}) {
        const auto transition = evaluateSessionTransition(state, true);

        EXPECT_TRUE(transition.should_leave_loop);
        EXPECT_TRUE(transition.should_release_swapchains);
        EXPECT_FALSE(transition.is_frame_loop_active);
    }
}

TEST(SessionStateTest, IdleStopsFramesWithoutLeavingTheLoop) {
    const auto transition = evaluateSessionTransition(SessionState::eIdle, false);

    EXPECT_FALSE(transition.is_frame_loop_active);
    EXPECT_FALSE(transition.should_leave_loop);
    EXPECT_FALSE(transition.should_begin_session);
}

}  // namespace vne::xr
