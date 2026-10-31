/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "render/text_renderer.h"

#include "launcher/bitmap_font.h"
#include "launcher/text_mesh.h"
#include "render/eye_pipeline.h"
#include "render/font_layout.h"
#include "render/text_sdf_spirv.h"

#include "vertexnova/logging/logging.h"
#include "vertexnova/rhi/blit_command_encoder.h"
#include "vertexnova/rhi/command_buffer.h"
#include "vertexnova/rhi/command_queue.h"
#include "vertexnova/rhi/shader_package.h"
#include "vertexnova/rhi/vertex_layout.h"

#include <cstring>
#include <fstream>
#include <iterator>

namespace vne::xr::samples {

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.xr.samples.text");

struct alignas(16) PushConstants {
    float mvp[16];
    float color[4];
    float params[4];  // distance range, atlas width, atlas height
};

// Capital letters in the built-in pixel font are 5 pixels tall; Roboto's are about 0.71 em.
constexpr float kPixelFontCapEm = 0.711f;

constexpr const char* kFontCandidates[] = {
    "/system/fonts/RobotoStatic-Regular.ttf",           // Android (Quest, Galaxy XR)
    "/system/fonts/Roboto-Regular.ttf",                 // Android, older layouts
    "/System/Library/Fonts/Supplemental/Arial.ttf",     // macOS
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",  // Linux
    "C:\\Windows\\Fonts\\arial.ttf",                    // Windows
};

/** A non-owning shared_ptr: the object's lifetime stays with its ResourceRef in the device pool. */
template<typename T>
std::shared_ptr<T> borrow(const vne::rhi::ResourceRef<T>& ref) {
    return std::shared_ptr<T>(ref.get(), [](T*) {});
}

template<typename T>
void destroyRef(vne::rhi::IDevice& device, vne::rhi::ResourceRef<T>& ref) {
    if (ref.handle().isValid()) {
        device.destroyResource(ref.handle());
    }
    ref.reset();
}

}  // namespace

TextRenderer::~TextRenderer() {
    shutdown();
}

bool TextRenderer::loadFontAtlas() {
    for (const char* path : kFontCandidates) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            continue;
        }
        const std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (bakeFontAtlas(data.data(), data.size(), atlas_)) {
            VNE_LOG_INFO << "Text: baked a " << atlas_.width << "x" << atlas_.height << " distance field atlas from "
                         << path;
            return true;
        }
        VNE_LOG_WARN << "Text: could not bake " << path;
    }
    return false;
}

bool TextRenderer::uploadAtlas() {
    using namespace vne::rhi;
    // Rows of the staging buffer are padded to 256 bytes (the copy's row alignment).
    constexpr std::size_t kRowAlign = 256;
    const std::size_t row_bytes = static_cast<std::size_t>(atlas_.width) * 4;
    const std::size_t padded_row = (row_bytes + kRowAlign - 1) & ~(kRowAlign - 1);
    std::vector<std::uint8_t> staging_data(padded_row * atlas_.height, 0);
    for (std::uint32_t row = 0; row < atlas_.height; ++row) {
        std::memcpy(staging_data.data() + row * padded_row,
                    atlas_.rgba.data() + static_cast<std::size_t>(row) * row_bytes,
                    row_bytes);
    }

    BufferDescriptor staging_desc{};
    staging_desc.name = "text_atlas_staging";
    staging_desc.size = staging_data.size();
    staging_desc.bind_flags = BufferBindFlags::eCopySrc;
    staging_desc.storage = ResourceStorage::eShared;
    staging_desc.cpu_access = CpuBufferAccessFlags::eNone;
    staging_desc.initial_data = staging_data.data();
    auto staging = device_->createBuffer(staging_desc);
    if (!staging) {
        return false;
    }

    TextureDescriptor texture_desc{};
    texture_desc.name = "text_atlas";
    texture_desc.type = TextureType::eTexture2d;
    // Linear (not sRGB): the field is data, not color.
    texture_desc.format = TextureFormat::eRgba8;
    texture_desc.width = atlas_.width;
    texture_desc.height = atlas_.height;
    texture_desc.mip_levels = 1;
    texture_desc.usage = TextureUsage::eSampled | TextureUsage::eTransferDst;
    texture_desc.storage = ResourceStorage::ePrivate;
    atlas_texture_ = device_->createTexture(texture_desc);
    if (!atlas_texture_) {
        destroyRef(*device_, staging);
        return false;
    }

    bool is_uploaded = false;
    if (auto queue = device_->createCommandQueue({.name = "text_atlas_upload"})) {
        if (auto cmd = queue->createCommandBuffer()) {
            if (cmd->begin()) {
                if (auto* blit = cmd->beginBlitPass()) {
                    const bool is_copied = blit->copyBufferToTexture(staging.get(),
                                                                     0,
                                                                     padded_row,
                                                                     0,
                                                                     BlitExtent3D{atlas_.width, atlas_.height, 1},
                                                                     atlas_texture_.get(),
                                                                     0,
                                                                     0,
                                                                     {});
                    blit->end();
                    cmd->endBlitPass(blit);
                    if (is_copied && cmd->end() && queue->submit(*cmd)) {
                        queue->waitIdle();
                        is_uploaded = true;
                    }
                }
            }
        }
    }
    destroyRef(*device_, staging);
    if (!is_uploaded) {
        VNE_LOG_ERROR << "Text: uploading the font atlas failed";
        return false;
    }

    SamplerDescriptor sampler_desc{};
    sampler_desc.name = "text_atlas_sampler";
    sampler_desc.min_filter = SamplerMinMagFilterMode::eLinear;
    sampler_desc.mag_filter = SamplerMinMagFilterMode::eLinear;
    sampler_desc.mipmap_filter = SamplerMipMapFilterMode::eNone;
    sampler_desc.address_mode_u = SamplerAddressMode::eClampToEdge;
    sampler_desc.address_mode_v = SamplerAddressMode::eClampToEdge;
    sampler_desc.address_mode_w = SamplerAddressMode::eClampToEdge;
    sampler_ = device_->createSampler(sampler_desc);
    if (!sampler_) {
        return false;
    }

    const std::vector<BindingDescriptor> bindings = {
        {"u_atlas", ReflectedResourceType::eSampledImage, 0, 0, ShaderStageFlags::eFragment},
        {"u_sampler", ReflectedResourceType::eSampler, 0, 1, ShaderStageFlags::eFragment},
    };
    resources_ = std::make_unique<ShaderResourceSet>(bindings, device_->getBackend());
    resources_->bindTexture(0, 0, borrow(atlas_texture_));
    resources_->bindSampler(0, 1, borrow(sampler_));
    return true;
}

bool TextRenderer::buildPipeline(vne::rhi::TextureFormat color,
                                 vne::rhi::TextureFormat depth,
                                 vne::rhi::SampleCount samples) {
    using namespace vne::rhi;
    destroyPipeline();

    ShaderPackage package{};
    package.name = "text_sdf";
    const auto add_stage = [&package](ShaderStage stage, const std::uint8_t* data, std::size_t size) {
        ShaderStageDescriptor desc{};
        desc.stage = stage;
        desc.entry_point = "main";
        ShaderBinaryData binary{};
        binary.bin_type = ShaderBinary::eSpirv;
        binary.data.assign(data, data + size);
        desc.payload = std::move(binary);
        package.stages.push_back(std::move(desc));
    };
    add_stage(ShaderStage::eVertex, text_sdf_spirv::kVertSpv, sizeof(text_sdf_spirv::kVertSpv));
    add_stage(ShaderStage::eFragment, text_sdf_spirv::kFragSpv, sizeof(text_sdf_spirv::kFragSpv));

    shader_library_ = device_->createShaderLibrary("text_sdf_shaders");
    if (!shader_library_.handle().isValid() || !shader_library_.get()->initialize(package)) {
        VNE_LOG_ERROR << "Text: shader library failed to initialize";
        destroyPipeline();
        return false;
    }

    VertexLayoutDescriptor layout{};
    layout.name = "text_sdf_layout";
    layout.attributes.push_back(
        VertexAttribute{"position", 0, 0, offsetof(FontVertex, position), VertexFormat::eFloat3});
    layout.attributes.push_back(VertexAttribute{"uv", 1, 0, offsetof(FontVertex, uv), VertexFormat::eFloat2});
    layout.bindings.push_back(VertexBinding{0, sizeof(FontVertex), 0});

    RenderPipelineDescriptor desc{};
    desc.name = "text_sdf_pipeline";
    desc.shader = shader_library_.get();
    desc.vertex_layout = layout;
    EyeRenderContext target{};
    target.color_format = color;
    target.depth_format = depth;
    target.sample_count = samples;
    setEyeTargetFormats(target, desc);
    desc.pipeline_state.cull_mode = CullMode::eNone;
    desc.pipeline_state.depth_test_enabled = depth != TextureFormat::eUnknown;
    // Text lies on a panel: it must be hidden by nearer things but never hide anything itself.
    desc.pipeline_state.depth_write_enabled = false;
    // Premultiplied alpha, leaving the destination alpha alone (the eye image is opaque).
    desc.pipeline_state.blend_state.enabled = true;
    desc.pipeline_state.blend_state.src_factor = BlendFactor::eOne;
    desc.pipeline_state.blend_state.dst_factor = BlendFactor::eOneMinusSrcAlpha;
    desc.pipeline_state.blend_state.src_alpha_factor = BlendFactor::eZero;
    desc.pipeline_state.blend_state.dst_alpha_factor = BlendFactor::eOne;

    pipeline_ = device_->createRenderPipeline(desc);
    if (!pipeline_) {
        VNE_LOG_ERROR << "Text: pipeline creation failed";
        destroyPipeline();
        return false;
    }
    pipeline_color_ = color;
    pipeline_depth_ = depth;
    pipeline_samples_ = samples;
    return true;
}

void TextRenderer::destroyPipeline() {
    if (!device_) {
        return;
    }
    destroyRef(*device_, pipeline_);
    destroyRef(*device_, shader_library_);
    pipeline_color_ = vne::rhi::TextureFormat::eUnknown;
    pipeline_depth_ = vne::rhi::TextureFormat::eUnknown;
}

void TextRenderer::releaseGpu() {
    if (!device_) {
        return;
    }
    destroyPipeline();
    resources_.reset();
    destroyRef(*device_, sampler_);
    destroyRef(*device_, atlas_texture_);
}

bool TextRenderer::initialize(const std::shared_ptr<vne::rhi::IDevice>& device) {
    shutdown();
    device_ = device;
    if (!device_) {
        return false;
    }
    if (!pixel_renderer_.initialize(device_)) {
        VNE_LOG_ERROR << "Text: the pixel font fallback could not be set up";
    }

    if (loadFontAtlas() && uploadAtlas()) {
        // Probe the pipeline now with typical eye formats, so a failure is found here and the pixel font is used for
        // every label, rather than meshes built for one font and drawn with the other.
        is_sdf_ready_ = buildPipeline(vne::rhi::TextureFormat::eSrgb8Alpha8,
                                      vne::rhi::TextureFormat::eDepth32f,
                                      vne::rhi::SampleCount::e1);
        destroyPipeline();
    }
    if (!is_sdf_ready_) {
        releaseGpu();
        VNE_LOG_WARN << "Text: using the built-in pixel font";
    } else {
        VNE_LOG_INFO << "Text: using the distance field font";
    }
    return true;
}

void TextRenderer::shutdown() {
    if (device_) {
        releaseGpu();
        pixel_renderer_.shutdown();
    }
    device_.reset();
    atlas_ = {};
    is_sdf_ready_ = false;
}

bool TextRenderer::prepare(const EyeRenderContext& context) {
    const bool is_pixel_ready = pixel_renderer_.prepare(context);
    if (!is_sdf_ready_) {
        return is_pixel_ready;
    }
    if (pipeline_ && context.color_format == pipeline_color_ && context.depth_format == pipeline_depth_
        && context.sample_count == pipeline_samples_) {
        return true;
    }
    return buildPipeline(context.color_format, context.depth_format, context.sample_count);
}

float TextRenderer::capHeight(float em_meters) const {
    return (is_sdf_ready_ ? atlas_.cap_height : kPixelFontCapEm) * em_meters;
}

float TextRenderer::measure(std::string_view text, float em_meters) const {
    if (is_sdf_ready_) {
        return measureFontText(atlas_, text, em_meters);
    }
    return measureText(text, capHeight(em_meters) / static_cast<float>(kGlyphHeight));
}

std::vector<std::string> TextRenderer::wrap(std::string_view text,
                                            float em_meters,
                                            float max_width,
                                            std::size_t max_lines) const {
    return wrapTextToWidth(text, max_width, max_lines, [this, em_meters](std::string_view s) {
        return measure(s, em_meters);
    });
}

TextMesh TextRenderer::createMesh(std::string_view text, float em_meters) const {
    TextMesh mesh;
    if (!device_) {
        return mesh;
    }
    mesh.width = measure(text, em_meters);
    if (!is_sdf_ready_) {
        const TextGeometry geometry = buildTextGeometry(text, capHeight(em_meters) / static_cast<float>(kGlyphHeight));
        if (!geometry.vertices.empty()) {
            mesh.pixel = pixel_renderer_.createMesh(geometry.vertices, geometry.indices);
        }
        return mesh;
    }

    const FontGeometry geometry = layoutFontText(atlas_, text, em_meters);
    if (geometry.vertices.empty()) {
        return mesh;
    }
    vne::rhi::BufferDescriptor vb_desc{};
    vb_desc.name = "text_vertices";
    vb_desc.size = geometry.vertices.size() * sizeof(FontVertex);
    vb_desc.bind_flags = vne::rhi::BufferBindFlags::eVertex | vne::rhi::BufferBindFlags::eCopyDst;
    vb_desc.storage = vne::rhi::ResourceStorage::eShared;
    vb_desc.initial_data = geometry.vertices.data();
    mesh.vertex_buffer = device_->createBuffer(vb_desc);

    vne::rhi::BufferDescriptor ib_desc{};
    ib_desc.name = "text_indices";
    ib_desc.size = geometry.indices.size() * sizeof(std::uint16_t);
    ib_desc.bind_flags = vne::rhi::BufferBindFlags::eIndex | vne::rhi::BufferBindFlags::eCopyDst;
    ib_desc.storage = vne::rhi::ResourceStorage::eShared;
    ib_desc.initial_data = geometry.indices.data();
    mesh.index_buffer = device_->createBuffer(ib_desc);

    if (!mesh.vertex_buffer || !mesh.index_buffer) {
        destroyMesh(mesh);
        return mesh;
    }
    mesh.index_count = static_cast<std::uint32_t>(geometry.indices.size());
    return mesh;
}

void TextRenderer::destroyMesh(TextMesh& mesh) const {
    if (device_) {
        pixel_renderer_.destroyMesh(mesh.pixel);
        if (mesh.vertex_buffer.handle().isValid()) {
            device_->destroyResource(mesh.vertex_buffer.handle());
        }
        if (mesh.index_buffer.handle().isValid()) {
            device_->destroyResource(mesh.index_buffer.handle());
        }
    }
    mesh = {};
}

void TextRenderer::draw(const EyeRenderContext& context,
                        const TextMesh& mesh,
                        const xrmath::Mat4& model,
                        const float color[4]) const {
    if (mesh.pixel.isValid()) {
        pixel_renderer_.draw(context, mesh.pixel, model, color);
        return;
    }
    if (!is_sdf_ready_ || !pipeline_ || mesh.index_count == 0 || context.encoder == nullptr || !resources_) {
        return;
    }
    auto* encoder = context.encoder;
    encoder->bindPipeline(pipeline_.get());
    encoder->bindResources(0, *resources_);
    encoder->bindVertexBuffer(0, mesh.vertex_buffer.get(), 0);
    encoder->bindIndexBuffer(mesh.index_buffer.get(), vne::rhi::IndexType::eUint16, 0);

    PushConstants pc{};
    const xrmath::Mat4 mvp = xrmath::multiply(context.view_proj, model);
    std::memcpy(pc.mvp, mvp.data(), sizeof(pc.mvp));
    std::memcpy(pc.color, color, sizeof(pc.color));
    pc.params[0] = atlas_.distance_range;
    pc.params[1] = static_cast<float>(atlas_.width);
    pc.params[2] = static_cast<float>(atlas_.height);
    encoder->setPushConstants(vne::rhi::ShaderStageFlags::eVertex | vne::rhi::ShaderStageFlags::eFragment,
                              &pc,
                              sizeof(pc),
                              0);
    encoder->drawIndexed(vne::rhi::DrawIndexedArguments{.index_count = mesh.index_count, .instance_count = 1});
}

}  // namespace vne::xr::samples
