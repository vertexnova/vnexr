/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "render/xr_frame_renderer.h"

#include "vertexnova/logging/logging.h"

namespace vne::xr::samples {

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.xr.samples.frame_renderer");
}

XrFrameRenderer::~XrFrameRenderer() {
    detach();
}

bool XrFrameRenderer::attach(ISession& session) {
    detach();
    device_ = session.graphicsDevice();
    if (!device_) {
        VNE_LOG_ERROR << "Frame renderer: the session has no graphics device";
        return false;
    }
    vne::rhi::CommandQueueDescriptor queue_desc{};
    queue_desc.name = "xr_frame_queue";
    queue_ = device_->createCommandQueue(queue_desc);
    if (!queue_) {
        VNE_LOG_ERROR << "Frame renderer: failed to create the command queue";
        device_.reset();
        return false;
    }
    return true;
}

void XrFrameRenderer::detach() {
    if (queue_) {
        queue_->waitIdle();
    }
    in_flight_.reset();
    queue_.reset();
    device_.reset();
}

void XrFrameRenderer::fail(const char* reason) {
    ++failed_frames_;
    // One line per cause burst, not one per frame: a repeating failure must not flood the log.
    if (failed_frames_ == 1 || failed_frames_ % 360 == 0) {
        VNE_LOG_ERROR << "Frame not drawn (" << failed_frames_ << " so far): " << reason;
    }
}

bool XrFrameRenderer::render(const Frame& frame, const std::vector<RenderPart>& parts, const float clear_color[3]) {
    if (!device_ || !queue_) {
        fail("renderer is not attached");
        return false;
    }
    if (frame.surfaces.view_count == 0 || frame.surfaces.view_count > frame.view_count) {
        fail("frame has no views");
        return false;
    }
    for (std::uint32_t eye = 0; eye < frame.surfaces.view_count; ++eye) {
        if (frame.surfaces.views[eye].render_target == nullptr) {
            fail("an eye has no render target");
            return false;
        }
    }

    // The session waited for the GPU in beginFrame(), so the previous frame's command buffer is no longer in use.
    in_flight_.reset();

    auto cmd = queue_->createCommandBuffer();
    if (!cmd || !cmd->begin()) {
        fail("could not begin a command buffer");
        return false;
    }

    for (std::uint32_t eye = 0; eye < frame.surfaces.view_count; ++eye) {
        const ViewSurface& surface = frame.surfaces.views[eye];
        const View& view = frame.views[eye];

        vne::rhi::RenderPassBeginInfo pass{};
        pass.render_target = surface.render_target;
        pass.color_attachments[0].load_op = vne::rhi::LoadOp::eClear;
        // A multisampled attachment is resolved into the compositor image and never read again.
        pass.color_attachments[0].store_op = surface.sample_count == vne::rhi::SampleCount::e1
                                                 ? vne::rhi::StoreOp::eStore
                                                 : vne::rhi::StoreOp::eDontCare;
        pass.color_attachments[0].clear_value = {clear_color[0], clear_color[1], clear_color[2], 1.f};
        pass.depth_stencil.depth_load_op = vne::rhi::LoadOp::eClear;
        pass.depth_stencil.depth_store_op = vne::rhi::StoreOp::eDontCare;
        pass.depth_stencil.clear_depth = 1.f;

        auto* encoder = cmd->beginRenderPass(pass);
        if (encoder == nullptr) {
            fail("could not begin a render pass");
            return false;
        }
        encoder->setViewport(vne::rhi::Viewport{0.f,
                                                0.f,
                                                static_cast<float>(surface.width),
                                                static_cast<float>(surface.height),
                                                0.f,
                                                1.f});
        encoder->setScissor(vne::rhi::ScissorRect{0, 0, surface.width, surface.height});

        const xrmath::Mat4 view_projection = xrmath::multiply(xrmath::fromColumnMajor(view.projection_matrix),
                                                              xrmath::fromColumnMajor(view.view_matrix));
        for (const RenderPart& part : parts) {
            if (part.stack == nullptr) {
                continue;
            }
            EyeRenderContext context;
            context.eye_index = eye;
            context.view = &view;
            context.world_from_scene = part.world_from_scene;
            context.view_proj = xrmath::multiply(view_projection, part.world_from_scene);
            context.encoder = encoder;
            context.color_format = surface.color_format;
            context.depth_format = surface.depth_format;
            context.sample_count = surface.sample_count;
            context.swapchain_generation = surface.swapchain_generation;
            context.width = surface.width;
            context.height = surface.height;
            part.stack->onRenderEye(context);
        }
        cmd->endRenderPass(encoder);
    }

    if (!cmd->end()) {
        fail("could not end the command buffer");
        return false;
    }
    if (!queue_->submit(*cmd)) {
        fail("queue submit failed");
        return false;
    }
    in_flight_ = std::move(cmd);
    return true;
}

}  // namespace vne::xr::samples
