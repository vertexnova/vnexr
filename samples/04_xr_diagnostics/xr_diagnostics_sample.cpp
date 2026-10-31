/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

/**
 * @file xr_diagnostics_sample.cpp
 * @brief Sample xr_diagnostics -- reference scene for scale, tracking, input and frame timing.
 */

#include "xr_diagnostics_layer.h"

#include "app/sample_registry.h"

#include <memory>

namespace vne::xr::samples {

void registerXrDiagnosticsSample(SampleRegistry& registry) {
    registry.add({
        .name = "xr_diagnostics",
        .description = "Floor grid, 1 m cube, axes, button lights, frame-time strip and a greeting: run this first",
        .backend = BackendType::eOpenXr,
        .setup =
            [](XrLayerStack& stack) { stack.pushLayer(std::make_unique<XrDiagnosticsLayer>(stack.textRenderer())); },
        .accent_color = {0.30f, 0.85f, 0.45f},
    });
}

}  // namespace vne::xr::samples
