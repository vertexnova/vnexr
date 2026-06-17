#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include <cstdint>

namespace vne::xr_ns {

/** @brief Normalized controller / hand input snapshot. */
struct ControllerState {
    bool active = false;
    float position[3]{};
    float orientation[4]{0.f, 0.f, 0.f, 1.f};
    float trigger = 0.f;
    float grip = 0.f;
    bool click_a = false;
    bool click_b = false;
};

struct InputState {
    static constexpr std::uint32_t kMaxControllers = 2;
    ControllerState controllers[kMaxControllers]{};
};

}  // namespace vne::xr_ns
