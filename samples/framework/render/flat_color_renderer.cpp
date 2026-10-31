/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "render/flat_color_renderer.h"

#include "render/eye_pipeline.h"
#include "render/flat_color_spirv.h"

#include "vertexnova/logging/logging.h"
#include "vertexnova/rhi/shader_package.h"
#include "vertexnova/rhi/vertex_layout.h"

#include <array>
#include <cstring>

namespace vne::xr::samples {

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.xr.samples.flat_color");

struct alignas(16) PushConstants {
    float mvp[16];
    float color[4];
};

void appendCube(std::vector<FlatVertex>& vertices, std::vector<std::uint16_t>& indices) {
    struct Face {
        float normal[3];
        float u[3];
        float v[3];
    };
    constexpr std::array<Face, 6> kFaces = {{
        {{1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 0.f, 1.f}},
        {{-1.f, 0.f, 0.f}, {0.f, 0.f, 1.f}, {0.f, 1.f, 0.f}},
        {{0.f, 1.f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 0.f, 0.f}},
        {{0.f, -1.f, 0.f}, {1.f, 0.f, 0.f}, {0.f, 0.f, 1.f}},
        {{0.f, 0.f, 1.f}, {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}},
        {{0.f, 0.f, -1.f}, {0.f, 1.f, 0.f}, {1.f, 0.f, 0.f}},
    }};
    constexpr std::array<std::array<float, 2>, 4> kCorners = {{{-1.f, -1.f}, {1.f, -1.f}, {1.f, 1.f}, {-1.f, 1.f}}};
    for (const auto& face : kFaces) {
        const auto base = static_cast<std::uint16_t>(vertices.size());
        for (const auto& corner : kCorners) {
            FlatVertex vertex{};
            for (int axis = 0; axis < 3; ++axis) {
                vertex.position[axis] = face.normal[axis] + face.u[axis] * corner[0] + face.v[axis] * corner[1];
                vertex.normal[axis] = face.normal[axis];
            }
            vertices.push_back(vertex);
        }
        for (const int offset : {0, 1, 2, 0, 2, 3}) {
            indices.push_back(static_cast<std::uint16_t>(base + offset));
        }
    }
}

}  // namespace

FlatColorRenderer::~FlatColorRenderer() {
    shutdown();
}

bool FlatColorRenderer::initialize(const std::shared_ptr<vne::rhi::IDevice>& device) {
    shutdown();
    device_ = device;
    if (!device_) {
        return false;
    }

    std::vector<FlatVertex> vertices;
    std::vector<std::uint16_t> indices;
    appendCube(vertices, indices);
    cube_ = createMesh(vertices, indices);

    const std::vector<FlatVertex> quad_vertices = {
        {{-1.f, -1.f, 0.f}, {kFullyLitNormal[0], kFullyLitNormal[1], kFullyLitNormal[2]}},
        {{1.f, -1.f, 0.f}, {kFullyLitNormal[0], kFullyLitNormal[1], kFullyLitNormal[2]}},
        {{1.f, 1.f, 0.f}, {kFullyLitNormal[0], kFullyLitNormal[1], kFullyLitNormal[2]}},
        {{-1.f, 1.f, 0.f}, {kFullyLitNormal[0], kFullyLitNormal[1], kFullyLitNormal[2]}},
    };
    quad_ = createMesh(quad_vertices, {0, 1, 2, 0, 2, 3});

    if (!cube_.isValid() || !quad_.isValid()) {
        VNE_LOG_ERROR << "Flat-colour renderer: failed to create the built-in meshes";
        shutdown();
        return false;
    }
    return true;
}

void FlatColorRenderer::shutdown() {
    if (device_) {
        destroyMesh(cube_);
        destroyMesh(quad_);
        destroyPipeline();
    }
    device_.reset();
}

void FlatColorRenderer::destroyPipeline() {
    if (!device_) {
        return;
    }
    if (pipeline_.handle().isValid()) {
        device_->destroyResource(pipeline_.handle());
    }
    if (shader_library_.handle().isValid()) {
        device_->destroyResource(shader_library_.handle());
    }
    pipeline_.reset();
    shader_library_.reset();
    pipeline_color_format_ = vne::rhi::TextureFormat::eUnknown;
    pipeline_depth_format_ = vne::rhi::TextureFormat::eUnknown;
}

FlatMesh FlatColorRenderer::createMesh(const std::vector<FlatVertex>& vertices,
                                       const std::vector<std::uint16_t>& indices) const {
    FlatMesh mesh;
    if (!device_ || vertices.empty() || indices.empty()) {
        return mesh;
    }
    vne::rhi::BufferDescriptor vb_desc{};
    vb_desc.name = "flat_vertices";
    vb_desc.size = vertices.size() * sizeof(FlatVertex);
    vb_desc.bind_flags = vne::rhi::BufferBindFlags::eVertex | vne::rhi::BufferBindFlags::eCopyDst;
    vb_desc.storage = vne::rhi::ResourceStorage::eShared;
    vb_desc.initial_data = vertices.data();
    mesh.vertex_buffer = device_->createBuffer(vb_desc);

    vne::rhi::BufferDescriptor ib_desc{};
    ib_desc.name = "flat_indices";
    ib_desc.size = indices.size() * sizeof(std::uint16_t);
    ib_desc.bind_flags = vne::rhi::BufferBindFlags::eIndex | vne::rhi::BufferBindFlags::eCopyDst;
    ib_desc.storage = vne::rhi::ResourceStorage::eShared;
    ib_desc.initial_data = indices.data();
    mesh.index_buffer = device_->createBuffer(ib_desc);

    if (!mesh.vertex_buffer || !mesh.index_buffer) {
        destroyMesh(mesh);
        return {};
    }
    mesh.index_count = static_cast<std::uint32_t>(indices.size());
    return mesh;
}

void FlatColorRenderer::destroyMesh(FlatMesh& mesh) const {
    if (device_) {
        if (mesh.vertex_buffer.handle().isValid()) {
            device_->destroyResource(mesh.vertex_buffer.handle());
        }
        if (mesh.index_buffer.handle().isValid()) {
            device_->destroyResource(mesh.index_buffer.handle());
        }
    }
    mesh = {};
}

bool FlatColorRenderer::prepare(const EyeRenderContext& context) {
    if (!device_) {
        return false;
    }
    if (pipeline_ && context.color_format == pipeline_color_format_ && context.depth_format == pipeline_depth_format_
        && context.sample_count == pipeline_sample_count_) {
        return true;
    }
    destroyPipeline();

    vne::rhi::ShaderPackage pkg{};
    pkg.name = "flat_color";
    const auto add_stage = [&pkg](vne::rhi::ShaderStage stage, const std::uint8_t* data, std::size_t size) {
        vne::rhi::ShaderStageDescriptor desc{};
        desc.stage = stage;
        desc.entry_point = "main";
        vne::rhi::ShaderBinaryData bin{};
        bin.bin_type = vne::rhi::ShaderBinary::eSpirv;
        bin.data.assign(data, data + size);
        desc.payload = std::move(bin);
        pkg.stages.push_back(std::move(desc));
    };
    add_stage(vne::rhi::ShaderStage::eVertex, flat_color_spirv::kVertSpv, sizeof(flat_color_spirv::kVertSpv));
    add_stage(vne::rhi::ShaderStage::eFragment, flat_color_spirv::kFragSpv, sizeof(flat_color_spirv::kFragSpv));

    shader_library_ = device_->createShaderLibrary("flat_color_shaders");
    if (!shader_library_.handle().isValid() || !shader_library_.get()->initialize(pkg)) {
        VNE_LOG_ERROR << "Flat-colour renderer: shader library failed to initialize";
        destroyPipeline();
        return false;
    }

    vne::rhi::VertexLayoutDescriptor layout{};
    layout.name = "flat_color_layout";
    layout.attributes.push_back(
        vne::rhi::VertexAttribute{"position", 0, 0, offsetof(FlatVertex, position), vne::rhi::VertexFormat::eFloat3});
    layout.attributes.push_back(
        vne::rhi::VertexAttribute{"normal", 1, 0, offsetof(FlatVertex, normal), vne::rhi::VertexFormat::eFloat3});
    layout.bindings.push_back(vne::rhi::VertexBinding{0, sizeof(FlatVertex), 0});

    vne::rhi::RenderPipelineDescriptor desc{};
    desc.name = "flat_color_pipeline";
    desc.shader = shader_library_.get();
    desc.vertex_layout = layout;
    setEyeTargetFormats(context, desc);
    desc.pipeline_state.cull_mode = vne::rhi::CullMode::eNone;
    const bool has_depth = context.depth_format != vne::rhi::TextureFormat::eUnknown;
    desc.pipeline_state.depth_test_enabled = has_depth;
    desc.pipeline_state.depth_write_enabled = has_depth;

    pipeline_ = device_->createRenderPipeline(desc);
    if (!pipeline_) {
        VNE_LOG_ERROR << "Flat-colour renderer: pipeline creation failed";
        destroyPipeline();
        return false;
    }
    pipeline_color_format_ = context.color_format;
    pipeline_depth_format_ = context.depth_format;
    pipeline_sample_count_ = context.sample_count;
    VNE_LOG_INFO << "Flat-colour pipeline ready (color=" << static_cast<int>(context.color_format)
                 << ", depth=" << static_cast<int>(context.depth_format)
                 << ", samples=" << static_cast<std::uint32_t>(context.sample_count) << ")";
    return true;
}

void FlatColorRenderer::draw(const EyeRenderContext& context,
                             const FlatMesh& mesh,
                             const xrmath::Mat4& model,
                             const float color[4]) const {
    if (!pipeline_ || !mesh.isValid() || context.encoder == nullptr) {
        return;
    }
    auto* encoder = context.encoder;
    encoder->bindPipeline(pipeline_.get());
    encoder->bindVertexBuffer(0, mesh.vertex_buffer.get(), 0);
    encoder->bindIndexBuffer(mesh.index_buffer.get(), vne::rhi::IndexType::eUint16, 0);

    PushConstants pc{};
    const xrmath::Mat4 mvp = xrmath::multiply(context.view_proj, model);
    std::memcpy(pc.mvp, mvp.data(), sizeof(pc.mvp));
    std::memcpy(pc.color, color, sizeof(pc.color));
    encoder->setPushConstants(vne::rhi::ShaderStageFlags::eVertex, &pc, sizeof(pc), 0);
    encoder->drawIndexed(vne::rhi::DrawIndexedArguments{.index_count = mesh.index_count, .instance_count = 1});
}

void FlatColorRenderer::drawBox(const EyeRenderContext& context,
                                const xrmath::Mat4& model,
                                xrmath::Vec3 half_extents,
                                const float color[4]) const {
    const xrmath::Mat4 scale = xrmath::fromTrs({}, {}, half_extents);
    draw(context, cube_, xrmath::multiply(model, scale), color);
}

}  // namespace vne::xr::samples
