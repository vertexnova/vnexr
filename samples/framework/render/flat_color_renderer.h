#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "layers/xr_layer.h"

#include "vertexnova/rhi/buffer.h"
#include "vertexnova/rhi/render_pipeline.h"
#include "vertexnova/rhi/resource_ref.h"
#include "vertexnova/rhi/shader_library.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace vne::xr::samples {

/**
 * @brief Normal that the shader lights at full brightness (it points along the shader's fixed light).
 * @details UI meshes use it so text and panels show their exact colour regardless of orientation.
 */
inline constexpr float kFullyLitNormal[3] = {0.3420f, 0.8305f, 0.4397f};

/** @brief Vertex of a flat-colour mesh: position and the normal used for the fixed-light face shading. */
struct FlatVertex {
    float position[3];
    float normal[3];
};

/** @brief Static GPU mesh owned by a `FlatColorRenderer`. */
struct FlatMesh {
    vne::rhi::ResourceRef<vne::rhi::IBuffer> vertex_buffer;
    vne::rhi::ResourceRef<vne::rhi::IBuffer> index_buffer;
    std::uint32_t index_count = 0;

    [[nodiscard]] bool isValid() const { return index_count > 0 && vertex_buffer && index_buffer; }
};

/**
 * @brief Draws shaded single-colour meshes (boxes, quads, text) into an eye's open render pass.
 *
 * @details The pipeline is rebuilt when the eye buffers' formats or sample count change. Call `prepare()` once per
 * eye callback, then any number of `draw*()` calls.
 */
class FlatColorRenderer {
   public:
    FlatColorRenderer() = default;
    ~FlatColorRenderer();

    FlatColorRenderer(const FlatColorRenderer&) = delete;
    FlatColorRenderer& operator=(const FlatColorRenderer&) = delete;

    /** @brief Create the built-in unit cube (half extent 1) and unit quad (XY, half extent 1) meshes. */
    [[nodiscard]] bool initialize(const std::shared_ptr<vne::rhi::IDevice>& device);
    /** @brief Destroy every GPU object this renderer created. Safe to call repeatedly. */
    void shutdown();

    /** @brief Make the pipeline match @p context. @return False when it cannot draw (the caller skips drawing). */
    [[nodiscard]] bool prepare(const EyeRenderContext& context);

    [[nodiscard]] FlatMesh createMesh(const std::vector<FlatVertex>& vertices,
                                      const std::vector<std::uint16_t>& indices) const;
    void destroyMesh(FlatMesh& mesh) const;

    [[nodiscard]] const FlatMesh& cube() const { return cube_; }
    [[nodiscard]] const FlatMesh& quad() const { return quad_; }

    void draw(const EyeRenderContext& context,
              const FlatMesh& mesh,
              const xrmath::Mat4& model,
              const float color[4]) const;
    /** @brief Cube with half extents @p half_extents centred at the model origin. */
    void drawBox(const EyeRenderContext& context,
                 const xrmath::Mat4& model,
                 xrmath::Vec3 half_extents,
                 const float color[4]) const;

   private:
    void destroyPipeline();

    std::shared_ptr<vne::rhi::IDevice> device_;
    vne::rhi::ResourceRef<vne::rhi::IShaderLibrary> shader_library_;
    vne::rhi::ResourceRef<vne::rhi::IRenderPipeline> pipeline_;
    vne::rhi::TextureFormat pipeline_color_format_ = vne::rhi::TextureFormat::eUnknown;
    vne::rhi::TextureFormat pipeline_depth_format_ = vne::rhi::TextureFormat::eUnknown;
    vne::rhi::SampleCount pipeline_sample_count_ = vne::rhi::SampleCount::e1;
    FlatMesh cube_;
    FlatMesh quad_;
};

}  // namespace vne::xr::samples
