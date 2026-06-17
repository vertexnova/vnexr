#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "layers/xr_layer.h"
#include "vertexnova/xr/input.h"
#include "vertexnova/xr/types.h"

#include <array>
#include <cstdint>
#include <vector>

namespace vne::xr::samples {

struct Block {
    Pose pose{};
    float color[3]{1.f, 0.5f, 0.2f};
    bool grabbed = false;
};

/** @brief Khronos Ch.4 block interaction (grab / spawn / recolor). */
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

/** @brief Scene + per-eye surface hook layer for OpenXR stereo simulator. */
class StereoCubeSceneLayer final : public IXrLayer {
   public:
    StereoCubeSceneLayer();

    void onAttach() override;
    void onUpdate(const FrameParams& params, LayerParams& out_layers) override;

   private:
    void clearEyeSurface(const ViewSurface& surface, float r, float g, float b);

    StereoCubeScene scene_;
    std::uint64_t frames_rendered_ = 0;
#ifdef VNE_XR_WITH_RHI
    std::shared_ptr<vne::rhi::IDevice> device_;
#endif
};

}  // namespace vne::xr::samples
