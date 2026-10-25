#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   October 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

/**
 * @file session_state.h
 * @brief Backend-independent session lifecycle rules.
 */

#include "vertexnova/xr/types.h"

namespace vne::xr {

/** @brief What a backend must do when the runtime moves the session into a new state. */
struct SessionTransition {
    SessionState state = SessionState::eUnknown;
    /** @brief Begin the session now (READY). The caller sets the frame loop active only if beginning succeeded. */
    bool should_begin_session = false;
    /** @brief End the session now (STOPPING). */
    bool should_end_session = false;
    /** @brief Destroy the swapchains: the compositor stops using them once the session stops. */
    bool should_release_swapchains = false;
    /** @brief Whether frames may be waited, begun and ended after this transition. */
    bool is_frame_loop_active = false;
    /** @brief The application loop must stop (session lost or exiting). */
    bool should_leave_loop = false;
};

/**
 * @brief The actions a transition into @p next requires.
 * @param next Target state. `eUnknown` yields a transition that changes nothing.
 * @param is_session_begun True when the session has already been begun (READY can repeat after STOPPING).
 */
[[nodiscard]] constexpr SessionTransition evaluateSessionTransition(SessionState next, bool is_session_begun) noexcept {
    SessionTransition transition;
    transition.state = next;
    switch (next) {
        case SessionState::eReady:
            transition.should_begin_session = !is_session_begun;
            transition.is_frame_loop_active = is_session_begun;
            break;
        case SessionState::eSynchronized:
        case SessionState::eVisible:
        case SessionState::eFocused:
            transition.is_frame_loop_active = true;
            break;
        case SessionState::eStopping:
            transition.should_end_session = is_session_begun;
            transition.should_release_swapchains = true;
            break;
        case SessionState::eLossPending:
        case SessionState::eExiting:
            transition.should_release_swapchains = true;
            transition.should_leave_loop = true;
            break;
        case SessionState::eIdle:
        case SessionState::eUnknown:
            break;
    }
    return transition;
}

}  // namespace vne::xr
