#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "layers/xr_layer_stack.h"
#include "render/xr_frame_renderer.h"
#include "vertexnova/xr/session.h"

#include <functional>
#include <memory>
#include <string>

namespace vne::xr::samples {

/**
 * @brief Sample host: owns ISession + layer stack and runs the one frame loop every sample shares.
 *
 * Usage:
 *   XrSampleApp app(config);
 *   app.pushLayer(std::make_unique<MySceneLayer>());
 *   return app.run();
 */
class XrSampleApp final : public IRenderSession {
   public:
    /**
     * @brief Called at the top of every loop iteration, before events are polled.
     * @return False to ask the app to quit. Platforms use it to pump their own event queue (Android's looper).
     */
    using FramePump = std::function<bool(ISession&)>;

    explicit XrSampleApp(SessionConfig config);
    ~XrSampleApp() override;

    /** @brief The layer stack, for `SampleInfo::setup`. */
    [[nodiscard]] XrLayerStack& stack() { return stack_; }

    void pushLayer(std::unique_ptr<IXrLayer> layer);
    void pushOverlay(std::unique_ptr<IXrLayer> overlay);

    /** Optional hook before session loop (platform init, RHI device, etc.). */
    void setOnReady(std::function<void()> callback);

    /** @brief Install the platform event pump (see FramePump). */
    void setFramePump(FramePump pump);

    /** @brief Create the session and run the frame loop until it ends. @return Process exit code. */
    int run();

    void onSessionReady() override;
    void update(const FrameParams& params, LayerParams& out_layers) override;

   private:
    SessionConfig config_;
    std::unique_ptr<ISession> session_;
    XrLayerStack stack_;
    XrFrameRenderer renderer_;
    bool is_renderer_attached_ = false;
    std::function<void()> on_ready_;
    FramePump frame_pump_;
};

}  // namespace vne::xr::samples
