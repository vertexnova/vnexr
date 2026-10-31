#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "layers/xr_layer_stack.h"

#include "vertexnova/rhi/command_buffer.h"
#include "vertexnova/rhi/command_queue.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace vne::xr::samples {

/** @brief A layer stack drawn with its own scene-space transform. */
struct RenderPart {
    XrLayerStack* stack = nullptr;
    xrmath::Mat4 world_from_scene = xrmath::identity();
};

/**
 * @brief Draws one XR frame: one command buffer, one render pass per eye, one submit.
 *
 * @details Layers only issue draw calls (`IXrLayer::onRenderEye`). Both eyes are always cleared first, so an image
 * handed to the compositor is always fully defined. `render()` returns true only when every eye was recorded and the
 * submit succeeded; the caller submits the projection layer only then.
 */
class XrFrameRenderer {
   public:
    XrFrameRenderer() = default;
    ~XrFrameRenderer();

    XrFrameRenderer(const XrFrameRenderer&) = delete;
    XrFrameRenderer& operator=(const XrFrameRenderer&) = delete;

    [[nodiscard]] bool attach(ISession& session);
    void detach();

    [[nodiscard]] bool render(const Frame& frame, const std::vector<RenderPart>& parts, const float clear_color[3]);

    [[nodiscard]] std::uint32_t failedFrames() const { return failed_frames_; }

   private:
    void fail(const char* reason);

    std::shared_ptr<vne::rhi::IDevice> device_;
    std::shared_ptr<vne::rhi::ICommandQueue> queue_;
    /** Kept until the next frame: the GPU may still be executing it when render() returns. */
    std::shared_ptr<vne::rhi::ICommandBuffer> in_flight_;
    std::uint32_t failed_frames_ = 0;
    std::uint32_t last_failure_frame_ = 0;
};

}  // namespace vne::xr::samples
