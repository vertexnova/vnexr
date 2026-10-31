#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "render/flat_color_renderer.h"
#include "render/font_atlas.h"

#include "vertexnova/rhi/buffer.h"
#include "vertexnova/rhi/render_pipeline.h"
#include "vertexnova/rhi/sampler.h"
#include "vertexnova/rhi/shader_library.h"
#include "vertexnova/rhi/shader_resource_binding.h"
#include "vertexnova/rhi/texture.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace vne::xr::samples {

/** @brief A line of text, uploaded once and drawn every frame. */
struct TextMesh {
    FlatMesh pixel;  //!< Pixel-font fallback.
    vne::rhi::ResourceRef<vne::rhi::IBuffer> vertex_buffer;
    vne::rhi::ResourceRef<vne::rhi::IBuffer> index_buffer;
    std::uint32_t index_count = 0;
    float width = 0.f;  //!< Meters.

    [[nodiscard]] bool isValid() const { return pixel.isValid() || (index_count > 0 && vertex_buffer && index_buffer); }
};

/**
 * @brief Draws lines of text as sharp, scalable glyphs from a distance-field atlas.
 *
 * @details The atlas is baked once from a system TrueType font (Roboto on Android) and uploaded as a texture; text is
 * then built into static meshes and drawn with the same median-and-pixel-range shader as vnegfx's MSDF text, so a
 * vnegfx atlas can replace the baked one later. When no font file is found, or the GPU setup fails, it falls back
 * to a built-in pixel font so labels never disappear. All sizes are meters: `em_meters` is the height of one em, and
 * a mesh's origin is the left end of the cap line (text grows right, capitals hang down).
 */
class TextRenderer {
   public:
    TextRenderer() = default;
    ~TextRenderer();

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    /** @brief Load the font, upload the atlas, and probe the pipeline. Always leaves a usable (maybe pixel) renderer.
     */
    [[nodiscard]] bool initialize(const std::shared_ptr<vne::rhi::IDevice>& device);
    void shutdown();

    [[nodiscard]] bool usesDistanceField() const { return is_sdf_ready_; }

    /** @brief Make the pipeline(s) match the eye buffers. Call once per eye callback before `draw()`. */
    [[nodiscard]] bool prepare(const EyeRenderContext& context);

    [[nodiscard]] TextMesh createMesh(std::string_view text, float em_meters) const;
    void destroyMesh(TextMesh& mesh) const;
    [[nodiscard]] float measure(std::string_view text, float em_meters) const;
    /** @brief Height in meters of a capital letter at @p em_meters. */
    [[nodiscard]] float capHeight(float em_meters) const;
    [[nodiscard]] std::vector<std::string> wrap(std::string_view text,
                                                float em_meters,
                                                float max_width,
                                                std::size_t max_lines) const;

    void draw(const EyeRenderContext& context,
              const TextMesh& mesh,
              const xrmath::Mat4& model,
              const float color[4]) const;

   private:
    [[nodiscard]] bool loadFontAtlas();
    [[nodiscard]] bool uploadAtlas();
    [[nodiscard]] bool buildPipeline(vne::rhi::TextureFormat color,
                                     vne::rhi::TextureFormat depth,
                                     vne::rhi::SampleCount samples);
    void destroyPipeline();
    void releaseGpu();

    std::shared_ptr<vne::rhi::IDevice> device_;
    FontAtlas atlas_;
    FlatColorRenderer pixel_renderer_;
    bool is_sdf_ready_ = false;

    vne::rhi::ResourceRef<vne::rhi::ITexture> atlas_texture_;
    vne::rhi::ResourceRef<vne::rhi::ISampler> sampler_;
    std::unique_ptr<vne::rhi::ShaderResourceSet> resources_;
    vne::rhi::ResourceRef<vne::rhi::IShaderLibrary> shader_library_;
    vne::rhi::ResourceRef<vne::rhi::IRenderPipeline> pipeline_;
    vne::rhi::TextureFormat pipeline_color_ = vne::rhi::TextureFormat::eUnknown;
    vne::rhi::TextureFormat pipeline_depth_ = vne::rhi::TextureFormat::eUnknown;
    vne::rhi::SampleCount pipeline_samples_ = vne::rhi::SampleCount::e1;
};

}  // namespace vne::xr::samples
