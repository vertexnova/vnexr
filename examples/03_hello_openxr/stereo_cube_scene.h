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
#include "vertexnova/xr/types.h"

#include <cstdint>
#include <vector>

namespace vne::xr::examples {

/** @brief Colored block for Khronos Ch.4 interaction demo. */
struct Block {
    Pose pose{};
    float color[3]{1.f, 0.5f, 0.2f};
    bool grabbed = false;
};

/** @brief Grab/spawn/color interaction driven by InputState. */
class StereoCubeScene {
   public:
    void reset();
    void update(const InputState& input, float delta_seconds);

    [[nodiscard]] const std::vector<Block>& blocks() const { return blocks_; }

   private:
    std::vector<Block> blocks_;
    std::uint32_t next_color_index_ = 0;
    bool prev_spawn_pressed_[InputState::kMaxControllers]{};
    bool prev_grab_pressed_[InputState::kMaxControllers]{};
};

}  // namespace vne::xr::examples
