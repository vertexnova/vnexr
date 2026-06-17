/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "exit_after_frames_layer.h"

#include "vertexnova/logging/logging.h"

namespace vne::xr::samples {

ExitAfterFramesLayer::ExitAfterFramesLayer(std::uint64_t max_frames)
    : IXrLayer("ExitAfterFrames")
    , max_frames_(max_frames) {}

void ExitAfterFramesLayer::onUpdate(const FrameParams& params, LayerParams& out_layers) {
    VNE_LOG_INFO << "Null simulator frame " << params.frame.frame_index << " views=" << params.frame.view_count
                 << " should_render=" << params.frame.should_render;
    if (params.frame.frame_index + 1 >= max_frames_) {
        out_layers.request_exit = true;
    }
}

}  // namespace vne::xr::samples
