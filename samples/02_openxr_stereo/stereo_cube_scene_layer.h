#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "layers/xr_layer.h"
#include "vertexnova/xr/input.h"
#include "vertexnova/xr/types.h"

#include "vertexnova/rhi/buffer.h"
#include "vertexnova/rhi/command_queue.h"
#include "vertexnova/rhi/device.h"
#include "vertexnova/rhi/render_pipeline.h"
#include "vertexnova/rhi/render_target.h"
#include "vertexnova/rhi/resource_ref.h"
#include "vertexnova/rhi/shader_library.h"
#include "vertexnova/rhi/texture_view.h"

#include <array>
#include <cstdint>
#include <memory>
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

/** @brief Scene + per-eye stereo cube renderer for OpenXR. */
class StereoCubeSceneLayer final : public IXrLayer {
   public:
    StereoCubeSceneLayer();
    ~StereoCubeSceneLayer() override;

    void onAttach(ISession* session) override;
    void onDetach() override;
    void onUpdate(const FrameParams& params, LayerParams& out_layers) override;

   private:
    [[nodiscard]] bool ensureGraphics(const ViewSurface& surface);
    void destroyGraphics();
    void renderEye(const Frame& frame, std::uint32_t eye_index, float clear_r, float clear_g, float clear_b);

    std::shared_ptr<vne::rhi::IDevice> device_;
    std::shared_ptr<vne::rhi::ICommandQueue> queue_;
    vne::rhi::ResourceRef<vne::rhi::IBuffer> vertex_buffer_;
    vne::rhi::ResourceRef<vne::rhi::IBuffer> index_buffer_;
    vne::rhi::ResourceRef<vne::rhi::IShaderLibrary> shader_library_;
    vne::rhi::ResourceRef<vne::rhi::IRenderPipeline> render_pipeline_;
    vne::rhi::TextureFormat pipeline_color_format_ = vne::rhi::TextureFormat::eUnknown;
    vne::rhi::TextureFormat pipeline_depth_format_ = vne::rhi::TextureFormat::eUnknown;
    std::uint32_t index_count_ = 0;
    bool graphics_ready_ = false;

    StereoCubeScene scene_;
    std::uint64_t frames_rendered_ = 0;
};

}  // namespace vne::xr::samples
