#pragma once
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

#include "vertexnova/xr/input.h"
#include "vertexnova/xr/xr_error.h"

#include <openxr/openxr.h>

#include <array>
#include <cstring>

namespace vne::xr {

/** @brief Khronos Ch.4 action sets, bindings, poses, and haptics. */
class OpenXrInput final {
   public:
    OpenXrInput() = default;
    ~OpenXrInput();

    OpenXrInput(const OpenXrInput&) = delete;
    OpenXrInput& operator=(const OpenXrInput&) = delete;

    [[nodiscard]] Result create(XrInstance instance, XrSession session);
    void destroy();

    [[nodiscard]] Result poll(XrTime predicted_time, InputState& out_input);
    [[nodiscard]] Result applyHaptic(std::uint32_t hand_index, float strength, XrDuration duration) const;

   private:
    [[nodiscard]] Result createAction(XrActionSet action_set,
                                      const char* name,
                                      XrActionType type,
                                      XrAction& out_action);
    [[nodiscard]] Result suggestBindings(XrInstance instance);

    XrInstance instance_ = XR_NULL_HANDLE;
    XrSession session_ = XR_NULL_HANDLE;
    XrActionSet action_set_ = XR_NULL_HANDLE;
    XrAction grab_action_ = XR_NULL_HANDLE;
    XrAction spawn_action_ = XR_NULL_HANDLE;
    XrAction change_color_action_ = XR_NULL_HANDLE;
    XrAction palm_pose_action_ = XR_NULL_HANDLE;
    XrAction buzz_action_ = XR_NULL_HANDLE;
    std::array<XrSpace, InputState::kMaxControllers> palm_spaces_{};
};

}  // namespace vne::xr
