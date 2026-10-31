/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 *
 * @file framework_eye_pipeline_test.cpp
 * @brief Pipelines built for the XR eye buffers must agree with them on the sample count.
 * ----------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include "render/eye_pipeline.h"

namespace vne::xr::samples {

namespace {

EyeRenderContext eyeContext(vne::rhi::SampleCount samples) {
    EyeRenderContext context;
    context.color_format = vne::rhi::TextureFormat::eSrgb8Alpha8;
    context.depth_format = vne::rhi::TextureFormat::eDepth32f;
    context.sample_count = samples;
    return context;
}

}  // namespace

TEST(EyePipelineTest, MultisampledEyeBuffersGetAMultisampledRasterizer) {
    vne::rhi::RenderPipelineDescriptor desc;
    setEyeTargetFormats(eyeContext(vne::rhi::SampleCount::e4), desc);

    EXPECT_TRUE(desc.pipeline_state.multisample_enabled);
    EXPECT_EQ(desc.pipeline_state.sample_count, vne::rhi::SampleCount::e4);
    EXPECT_EQ(desc.formats.sample_count, vne::rhi::SampleCount::e4);
    EXPECT_EQ(desc.formats.color_formats[0], vne::rhi::TextureFormat::eSrgb8Alpha8);
    EXPECT_EQ(desc.formats.depth_stencil_format, vne::rhi::TextureFormat::eDepth32f);
}

TEST(EyePipelineTest, SingleSampledEyeBuffersKeepMultisamplingOff) {
    vne::rhi::RenderPipelineDescriptor desc;
    setEyeTargetFormats(eyeContext(vne::rhi::SampleCount::e1), desc);

    EXPECT_FALSE(desc.pipeline_state.multisample_enabled);
    EXPECT_EQ(desc.pipeline_state.sample_count, vne::rhi::SampleCount::e1);
    EXPECT_EQ(desc.formats.sample_count, vne::rhi::SampleCount::e1);
}

TEST(EyePipelineTest, TheRenderPassVnerhiBuildsForThePipelineMatchesTheEyeBuffers) {
    // vnerhi derives the pipeline's compatible render pass from attachmentFormatsOf(); setting only
    // formats.sample_count once made it a 1-sample pass used inside the 4-sample eye pass.
    for (const auto samples : {vne::rhi::SampleCount::e1, vne::rhi::SampleCount::e4}) {
        vne::rhi::RenderPipelineDescriptor desc;
        setEyeTargetFormats(eyeContext(samples), desc);

        EXPECT_EQ(vne::rhi::attachmentFormatsOf(desc), desc.formats);
        EXPECT_EQ(vne::rhi::attachmentFormatsOf(desc).sample_count, samples);
    }
}

}  // namespace vne::xr::samples
