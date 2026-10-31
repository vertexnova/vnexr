#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "layers/xr_layer.h"

#include "vertexnova/rhi/render_pipeline.h"

namespace vne::xr::samples {

/**
 * @brief Make @p desc render into the eye buffers described by @p context.
 *
 * @details Sets the attachment formats **and** the rasterizer sample count together. vnerhi builds a pipeline's
 * compatible render pass and its `rasterizationSamples` from `pipeline_state.sample_count` /
 * `pipeline_state.multisample_enabled`, not from `formats.sample_count` (see `vne::rhi::attachmentFormatsOf`).
 * Setting only the formats creates a 1-sample pipeline that is then drawn inside the 4-sample eye pass: invalid
 * Vulkan, which on Quest's GPU squeezes the whole image into a strip. Every pipeline that draws in
 * `IXrLayer::onRenderEye` should be described through this function.
 */
inline void setEyeTargetFormats(const EyeRenderContext& context, vne::rhi::RenderPipelineDescriptor& desc) {
    desc.formats.color_formats[0] = context.color_format;
    desc.formats.depth_stencil_format = context.depth_format;
    desc.formats.sample_count = context.sample_count;
    desc.pipeline_state.sample_count = context.sample_count;
    desc.pipeline_state.multisample_enabled = context.sample_count != vne::rhi::SampleCount::e1;
}

}  // namespace vne::xr::samples
