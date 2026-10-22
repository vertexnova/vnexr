/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "stereo_cube_scene_layer.h"

#include "stereo_cube_spirv.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "vertexnova/logging/logging.h"
#include "vertexnova/rhi/command_buffer.h"
#include "vertexnova/rhi/command_queue.h"
#include "vertexnova/rhi/render_command_encoder.h"
#include "vertexnova/rhi/shader_package.h"
#include "vertexnova/rhi/submission.h"
#include "vertexnova/rhi/texture.h"
#include "vertexnova/rhi/vertex_layout.h"

namespace vne::xr::samples {

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.xr.samples.stereo_cube");

constexpr std::array<std::array<float, 3>, 6> kPalette = {{
    {1.f, 0.2f, 0.2f},
    {0.2f, 1.f, 0.2f},
    {0.2f, 0.4f, 1.f},
    {1.f, 0.8f, 0.2f},
    {0.8f, 0.2f, 1.f},
    {0.2f, 1.f, 1.f},
}};

constexpr float kBlockHalfExtent = 0.05f;
constexpr float kTableHalfExtentX = 0.35f;
constexpr float kTableHalfExtentY = 0.02f;
constexpr float kTableHalfExtentZ = 0.25f;

struct CubeVertex {
    float position[3];
};

constexpr std::array<CubeVertex, 8> kCubeVertices = {{
    {{-1.f, -1.f, -1.f}},
    {{1.f, -1.f, -1.f}},
    {{1.f, 1.f, -1.f}},
    {{-1.f, 1.f, -1.f}},
    {{-1.f, -1.f, 1.f}},
    {{1.f, -1.f, 1.f}},
    {{1.f, 1.f, 1.f}},
    {{-1.f, 1.f, 1.f}},
}};

constexpr std::array<std::uint16_t, 36> kCubeIndices = {{
    0, 3, 2, 0, 2, 1, 4, 5, 6, 4, 6, 7, 0, 4, 7, 0, 7, 3, 1, 2, 6, 1, 6, 5, 3, 7, 6, 3, 6, 2, 0, 1, 5, 0, 5, 4,
}};

struct alignas(16) PushConstants {
    float mvp[16];
    float color[4];
};

void multiplyMat4(const float a[16], const float b[16], float out[16]) {
    float tmp[16]{};
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            tmp[col * 4 + row] = a[0 * 4 + row] * b[col * 4 + 0] + a[1 * 4 + row] * b[col * 4 + 1]
                                 + a[2 * 4 + row] * b[col * 4 + 2] + a[3 * 4 + row] * b[col * 4 + 3];
        }
    }
    std::memcpy(out, tmp, sizeof(tmp));
}

void poseToModelMatrix(const Pose& pose, float sx, float sy, float sz, float out[16]) {
    const float x = pose.orientation[0];
    const float y = pose.orientation[1];
    const float z = pose.orientation[2];
    const float w = pose.orientation[3];

    const float xx = x * x;
    const float yy = y * y;
    const float zz = z * z;
    const float xy = x * y;
    const float xz = x * z;
    const float yz = y * z;
    const float wx = w * x;
    const float wy = w * y;
    const float wz = w * z;

    // Column-major rotation * scale, then translation.
    out[0] = (1.f - 2.f * (yy + zz)) * sx;
    out[1] = (2.f * (xy + wz)) * sx;
    out[2] = (2.f * (xz - wy)) * sx;
    out[3] = 0.f;

    out[4] = (2.f * (xy - wz)) * sy;
    out[5] = (1.f - 2.f * (xx + zz)) * sy;
    out[6] = (2.f * (yz + wx)) * sy;
    out[7] = 0.f;

    out[8] = (2.f * (xz + wy)) * sz;
    out[9] = (2.f * (yz - wx)) * sz;
    out[10] = (1.f - 2.f * (xx + yy)) * sz;
    out[11] = 0.f;

    out[12] = pose.position[0];
    out[13] = pose.position[1];
    out[14] = pose.position[2];
    out[15] = 1.f;
}

}  // namespace

void StereoCubeScene::reset() {
    blocks_.clear();
    Block table{};
    table.pose.position[1] = 0.8f;
    table.color[0] = 0.35f;
    table.color[1] = 0.35f;
    table.color[2] = 0.4f;
    blocks_.push_back(table);

    for (int i = 0; i < 3; ++i) {
        Block block{};
        block.pose.position[0] = static_cast<float>(i - 1) * 0.15f;
        block.pose.position[1] = 1.0f;
        block.pose.position[2] = -0.4f;
        block.color[0] = kPalette[static_cast<std::size_t>(i)][0];
        block.color[1] = kPalette[static_cast<std::size_t>(i)][1];
        block.color[2] = kPalette[static_cast<std::size_t>(i)][2];
        blocks_.push_back(block);
    }
    next_color_index_ = 3;
}

void StereoCubeScene::update(const InputState& input, float delta_seconds) {
    (void)delta_seconds;
    const std::uint32_t count = std::min(input.controller_count, InputState::kMaxControllers);
    for (std::uint32_t hand = 0; hand < count; ++hand) {
        const bool spawn_now = input.select_pressed[hand];
        if (spawn_now && !prev_spawn_pressed_[hand] && input.pose_valid[hand]) {
            Block block{};
            block.pose = input.controller_poses[hand];
            block.pose.position[2] -= 0.1f;
            const auto& color = kPalette[next_color_index_ % kPalette.size()];
            block.color[0] = color[0];
            block.color[1] = color[1];
            block.color[2] = color[2];
            blocks_.push_back(block);
            ++next_color_index_;
        }
        prev_spawn_pressed_[hand] = spawn_now;

        const bool grab_now = input.squeeze_pressed[hand];
        if (grab_now && !prev_grab_pressed_[hand]) {
            float best_dist = 0.25f;
            Block* best = nullptr;
            for (auto& block : blocks_) {
                if (block.grabbed) {
                    continue;
                }
                const float dx = block.pose.position[0] - input.controller_poses[hand].position[0];
                const float dy = block.pose.position[1] - input.controller_poses[hand].position[1];
                const float dz = block.pose.position[2] - input.controller_poses[hand].position[2];
                const float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
                if (dist < best_dist) {
                    best_dist = dist;
                    best = &block;
                }
            }
            if (best != nullptr) {
                best->grabbed = true;
            }
        }
        if (!grab_now) {
            for (auto& block : blocks_) {
                block.grabbed = false;
            }
        }
        prev_grab_pressed_[hand] = grab_now;

        if (input.change_color_pressed[hand]) {
            for (auto& block : blocks_) {
                if (block.grabbed) {
                    const auto& color = kPalette[next_color_index_ % kPalette.size()];
                    block.color[0] = color[0];
                    block.color[1] = color[1];
                    block.color[2] = color[2];
                    ++next_color_index_;
                    break;
                }
            }
        }

        for (auto& block : blocks_) {
            if (block.grabbed && input.pose_valid[hand]) {
                block.pose = input.controller_poses[hand];
                block.pose.position[2] -= 0.05f;
            }
        }
    }
}

StereoCubeSceneLayer::StereoCubeSceneLayer()
    : IXrLayer("StereoCubeScene") {}

StereoCubeSceneLayer::~StereoCubeSceneLayer() {
    onDetach();
}

void StereoCubeSceneLayer::onAttach(ISession* session) {
    scene_.reset();
    destroyGraphics();
    if (session != nullptr) {
        device_ = session->graphicsDevice();
    }
    if (!device_) {
        VNE_LOG_WARN << "OpenXR stereo sample: session has no graphics device yet";
    } else {
        vne::rhi::CommandQueueDescriptor queue_desc{};
        queue_desc.name = "openxr_stereo_queue";
        queue_ = device_->createCommandQueue(queue_desc);
        if (!queue_) {
            VNE_LOG_ERROR << "OpenXR stereo sample: failed to create command queue";
        }
    }
    VNE_LOG_INFO << "OpenXR stereo sample ready";
}

void StereoCubeSceneLayer::onDetach() {
    destroyGraphics();
    queue_.reset();
    device_.reset();
}

void StereoCubeSceneLayer::destroyGraphics() {
    if (device_) {
        if (render_pipeline_.handle().isValid()) {
            device_->destroyResource(render_pipeline_.handle());
        }
        if (shader_library_.handle().isValid()) {
            device_->destroyResource(shader_library_.handle());
        }
        if (index_buffer_.handle().isValid()) {
            device_->destroyResource(index_buffer_.handle());
        }
        if (vertex_buffer_.handle().isValid()) {
            device_->destroyResource(vertex_buffer_.handle());
        }
    }
    render_pipeline_.reset();
    shader_library_.reset();
    index_buffer_.reset();
    vertex_buffer_.reset();
    pipeline_color_format_ = vne::rhi::TextureFormat::eUnknown;
    pipeline_depth_format_ = vne::rhi::TextureFormat::eUnknown;
    index_count_ = 0;
    graphics_ready_ = false;
}

bool StereoCubeSceneLayer::ensureGraphics(const ViewSurface& surface) {
    if (!device_ || !queue_ || !surface.color_texture) {
        return false;
    }

    const auto color_format = surface.color_texture->descriptor().format;
    const auto depth_format =
        surface.depth_texture ? surface.depth_texture->descriptor().format : vne::rhi::TextureFormat::eUnknown;
    if (graphics_ready_ && color_format == pipeline_color_format_ && depth_format == pipeline_depth_format_) {
        return true;
    }

    destroyGraphics();

    vne::rhi::BufferDescriptor vb_desc{};
    vb_desc.name = "stereo_cube_vertices";
    vb_desc.size = sizeof(kCubeVertices);
    vb_desc.bind_flags = vne::rhi::BufferBindFlags::eVertex | vne::rhi::BufferBindFlags::eCopyDst;
    vb_desc.storage = vne::rhi::ResourceStorage::eShared;
    vb_desc.initial_data = kCubeVertices.data();
    vertex_buffer_ = device_->createBuffer(vb_desc);
    if (!vertex_buffer_) {
        return false;
    }

    vne::rhi::BufferDescriptor ib_desc{};
    ib_desc.name = "stereo_cube_indices";
    ib_desc.size = sizeof(kCubeIndices);
    ib_desc.bind_flags = vne::rhi::BufferBindFlags::eIndex | vne::rhi::BufferBindFlags::eCopyDst;
    ib_desc.storage = vne::rhi::ResourceStorage::eShared;
    ib_desc.initial_data = kCubeIndices.data();
    index_buffer_ = device_->createBuffer(ib_desc);
    if (!index_buffer_) {
        return false;
    }
    index_count_ = static_cast<std::uint32_t>(kCubeIndices.size());

    vne::rhi::ShaderPackage pkg{};
    pkg.name = "stereo_cube";
    {
        vne::rhi::ShaderStageDescriptor vert{};
        vert.stage = vne::rhi::ShaderStage::eVertex;
        vert.entry_point = "main";
        vne::rhi::ShaderBinaryData bin{};
        bin.bin_type = vne::rhi::ShaderBinary::eSpirv;
        bin.data.assign(stereo_cube_spirv::kVertSpv, stereo_cube_spirv::kVertSpv + sizeof(stereo_cube_spirv::kVertSpv));
        vert.payload = std::move(bin);
        pkg.stages.push_back(std::move(vert));
    }
    {
        vne::rhi::ShaderStageDescriptor frag{};
        frag.stage = vne::rhi::ShaderStage::eFragment;
        frag.entry_point = "main";
        vne::rhi::ShaderBinaryData bin{};
        bin.bin_type = vne::rhi::ShaderBinary::eSpirv;
        bin.data.assign(stereo_cube_spirv::kFragSpv, stereo_cube_spirv::kFragSpv + sizeof(stereo_cube_spirv::kFragSpv));
        frag.payload = std::move(bin);
        pkg.stages.push_back(std::move(frag));
    }

    shader_library_ = device_->createShaderLibrary("stereo_cube_shaders");
    if (!shader_library_.handle().isValid() || !shader_library_.get()->initialize(pkg)) {
        VNE_LOG_ERROR << "Failed to initialize stereo cube shader library";
        return false;
    }

    vne::rhi::VertexLayoutDescriptor layout_desc{};
    layout_desc.name = "stereo_cube_layout";
    layout_desc.attributes.push_back(vne::rhi::VertexAttribute{"position", 0, 0, 0, vne::rhi::VertexFormat::eFloat3});
    layout_desc.bindings.push_back(vne::rhi::VertexBinding{0, sizeof(CubeVertex), 0});

    vne::rhi::RenderPipelineDescriptor pipeline_desc{};
    pipeline_desc.name = "stereo_cube_pipeline";
    pipeline_desc.shader = shader_library_.get();
    pipeline_desc.vertex_layout = layout_desc;
    pipeline_desc.formats.color_formats[0] = color_format;
    pipeline_desc.formats.depth_stencil_format = depth_format;
    pipeline_desc.formats.sample_count = vne::rhi::SampleCount::e1;
    pipeline_desc.pipeline_state.cull_mode = vne::rhi::CullMode::eNone;
    const bool has_depth = depth_format != vne::rhi::TextureFormat::eUnknown;
    pipeline_desc.pipeline_state.depth_test_enabled = has_depth;
    pipeline_desc.pipeline_state.depth_write_enabled = has_depth;

    render_pipeline_ = device_->createRenderPipeline(pipeline_desc);
    if (!render_pipeline_) {
        VNE_LOG_ERROR << "Failed to create stereo cube pipeline";
        return false;
    }

    pipeline_color_format_ = color_format;
    pipeline_depth_format_ = depth_format;
    graphics_ready_ = true;
    VNE_LOG_INFO << "Stereo cube graphics ready (color=" << static_cast<int>(color_format)
                 << ", depth=" << static_cast<int>(depth_format) << ")";
    return true;
}

void StereoCubeSceneLayer::renderEye(
    const Frame& frame, std::uint32_t eye_index, float clear_r, float clear_g, float clear_b) {
    if (!graphics_ready_ || eye_index >= frame.surfaces.view_count || eye_index >= frame.view_count) {
        return;
    }
    const auto& surface = frame.surfaces.views[eye_index];
    const auto& view = frame.views[eye_index];
    if (!surface.color_texture) {
        return;
    }

    vne::rhi::TextureViewDescriptor color_view_desc{};
    color_view_desc.name = "eye_color_view";
    color_view_desc.texture = surface.color_texture;
    color_view_desc.bind_flags = static_cast<vne::rhi::TextureViewBindFlags>(
        static_cast<std::uint32_t>(vne::rhi::TextureViewBindFlags::eViewBindColorAttachment)
        | static_cast<std::uint32_t>(vne::rhi::TextureViewBindFlags::eViewBindSampled));
    auto color_view = device_->createTextureView(color_view_desc);
    if (!color_view) {
        return;
    }

    vne::rhi::ResourceRef<vne::rhi::ITextureView> depth_view{};
    if (surface.depth_texture) {
        vne::rhi::TextureViewDescriptor depth_view_desc{};
        depth_view_desc.name = "eye_depth_view";
        depth_view_desc.texture = surface.depth_texture;
        depth_view_desc.bind_flags = static_cast<vne::rhi::TextureViewBindFlags>(
            static_cast<std::uint32_t>(vne::rhi::TextureViewBindFlags::eViewBindDepthAttachment)
            | static_cast<std::uint32_t>(vne::rhi::TextureViewBindFlags::eViewBindSampled));
        depth_view = device_->createTextureView(depth_view_desc);
        if (!depth_view) {
            device_->destroyResource(color_view.handle());
            return;
        }
    }

    vne::rhi::RenderTargetDescriptor target_desc{};
    target_desc.name = "eye_target";
    target_desc.width = surface.width;
    target_desc.height = surface.height;
    target_desc.formats.color_formats[0] = pipeline_color_format_;
    target_desc.formats.depth_stencil_format = pipeline_depth_format_;
    target_desc.formats.sample_count = vne::rhi::SampleCount::e1;
    target_desc.color_attachments[0].texture_view = color_view;
    target_desc.depth_stencil.texture_view = depth_view;

    auto render_target = device_->createRenderTarget(target_desc);
    if (!render_target) {
        if (depth_view.handle().isValid()) {
            device_->destroyResource(depth_view.handle());
        }
        device_->destroyResource(color_view.handle());
        return;
    }

    auto cmd = queue_->createCommandBuffer();
    if (!cmd || !cmd->begin()) {
        device_->destroyResource(render_target.handle());
        if (depth_view.handle().isValid()) {
            device_->destroyResource(depth_view.handle());
        }
        device_->destroyResource(color_view.handle());
        return;
    }

    vne::rhi::RenderPassBeginInfo pass{};
    pass.render_target = render_target.get();
    pass.color_attachments[0].load_op = vne::rhi::LoadOp::eClear;
    pass.color_attachments[0].store_op = vne::rhi::StoreOp::eStore;
    pass.color_attachments[0].clear_value = {clear_r, clear_g, clear_b, 1.f};
    if (depth_view) {
        pass.depth_stencil.depth_load_op = vne::rhi::LoadOp::eClear;
        pass.depth_stencil.depth_store_op = vne::rhi::StoreOp::eDontCare;
        pass.depth_stencil.clear_depth = 1.f;
    }

    auto* encoder = cmd->beginRenderPass(pass);
    if (encoder != nullptr) {
        encoder->setViewport(vne::rhi::Viewport{0.f,
                                                0.f,
                                                static_cast<float>(surface.width),
                                                static_cast<float>(surface.height),
                                                0.f,
                                                1.f});
        encoder->setScissor(vne::rhi::ScissorRect{0, 0, surface.width, surface.height});
        encoder->bindPipeline(render_pipeline_.get());
        encoder->bindVertexBuffer(0, vertex_buffer_.get(), 0);
        encoder->bindIndexBuffer(index_buffer_.get(), vne::rhi::IndexType::eUint16, 0);

        float view_proj[16]{};
        multiplyMat4(view.projection_matrix, view.view_matrix, view_proj);

        const auto& blocks = scene_.blocks();
        for (std::size_t i = 0; i < blocks.size(); ++i) {
            const auto& block = blocks[i];
            const bool is_table = (i == 0);
            const float sx = is_table ? kTableHalfExtentX : kBlockHalfExtent;
            const float sy = is_table ? kTableHalfExtentY : kBlockHalfExtent;
            const float sz = is_table ? kTableHalfExtentZ : kBlockHalfExtent;

            float model[16]{};
            poseToModelMatrix(block.pose, sx, sy, sz, model);

            PushConstants pc{};
            multiplyMat4(view_proj, model, pc.mvp);
            pc.color[0] = block.color[0];
            pc.color[1] = block.color[1];
            pc.color[2] = block.color[2];
            pc.color[3] = 1.f;

            encoder->setPushConstants(vne::rhi::ShaderStageFlags::eVertex, &pc, sizeof(pc), 0);
            encoder->drawIndexed(vne::rhi::DrawIndexedArguments{.index_count = index_count_, .instance_count = 1});
        }
        cmd->endRenderPass(encoder);
    }

    cmd->end();
    (void)queue_->submit(*cmd);

    device_->destroyResource(render_target.handle());
    if (depth_view.handle().isValid()) {
        device_->destroyResource(depth_view.handle());
    }
    device_->destroyResource(color_view.handle());
}

void StereoCubeSceneLayer::onUpdate(const FrameParams& params, LayerParams& out_layers) {
    if (!params.frame.should_render) {
        return;
    }

    scene_.update(params.input, 1.f / 72.f);
    const auto& blocks = scene_.blocks();

    float clear_r = 0.05f;
    float clear_g = 0.05f;
    float clear_b = 0.08f;
    if (blocks.size() > 1) {
        clear_r = blocks[1].color[0] * 0.2f;
        clear_g = blocks[1].color[1] * 0.2f;
        clear_b = blocks[1].color[2] * 0.2f;
    }

    if (params.frame.surfaces.view_count > 0) {
        if (!ensureGraphics(params.frame.surfaces.views[0])) {
            VNE_LOG_WARN << "Stereo cube graphics not ready; skipping frame";
        } else {
            for (std::uint32_t i = 0; i < params.frame.surfaces.view_count; ++i) {
                const float eye_tint = (i == 0) ? 0.02f : -0.02f;
                renderEye(params.frame, i, clear_r + eye_tint, clear_g, clear_b + eye_tint);
            }
        }
    }

    ++frames_rendered_;
    if (frames_rendered_ >= 3600) {
        out_layers.request_exit = true;
    }
}

}  // namespace vne::xr::samples
