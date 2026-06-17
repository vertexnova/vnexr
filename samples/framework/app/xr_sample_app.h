#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "layers/xr_layer_stack.h"
#include "vertexnova/xr/session.h"

#include <functional>
#include <memory>
#include <string>

namespace vne::xr::samples {

/**
 * @brief Sample host: owns ISession + layer stack (vnegfx SampleApplication analogue).
 *
 * Usage:
 *   XrSampleApp app(config);
 *   app.pushLayer(std::make_unique<MySceneLayer>());
 *   return app.run();
 */
class XrSampleApp final : public IRenderSession {
   public:
    explicit XrSampleApp(SessionConfig config);
    ~XrSampleApp() override;

    void pushLayer(std::unique_ptr<IXrLayer> layer);
    void pushOverlay(std::unique_ptr<IXrLayer> overlay);

    /** Optional hook before session loop (platform init, RHI device, etc.). */
    void setOnReady(std::function<void()> callback);

    int run();

    void onSessionReady() override;
    void update(const FrameParams& params, LayerParams& out_layers) override;

   private:
    SessionConfig config_;
    std::unique_ptr<ISession> session_;
    XrLayerStack stack_;
    std::function<void()> on_ready_;
};

}  // namespace vne::xr::samples
