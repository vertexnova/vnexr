#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   June 2026
 *
 * Autodoc:   yes
 * ---------------------------------------------------------------------- */

/**
 * @file session.h
 * @brief Platform-agnostic XR session lifecycle.
 */

#include "vertexnova/xr/frame.h"
#include "vertexnova/xr/input.h"
#include "vertexnova/xr/render_session.h"
#include "vertexnova/xr/types.h"
#include "vertexnova/rhi/device.h"

#include <memory>
#include <string>

namespace vne::xr {

/** @brief Optional platform handles for OpenXR instance creation. */
struct SessionPlatformHandle {
#if defined(_WIN32)
    void* hwnd = nullptr;
    void* hinstance = nullptr;
#endif
#if defined(__ANDROID__)
    void* application_vm = nullptr;
    void* application_context = nullptr;
#endif
};

struct SessionConfig {
    BackendType backend = BackendType::eNull;
    std::string application_name = "VneXR";
    SessionPlatformHandle platform{};
    /** @brief MSAA samples for the eye buffers (1, 2 or 4). The backend falls back to 1 when unsupported. */
    std::uint32_t color_sample_count = 4;
    /**
     * @brief Wait for the GPU to finish the frame before releasing its image to the compositor.
     * @details On by default: without it the headset showed random pixel blocks while the scene was being moved,
     * the signature of the compositor reading an image the GPU had not finished. Costs a little latency.
     */
    bool is_gpu_sync_every_frame = true;
    /** @brief Eye buffer size relative to the runtime's recommendation, clamped to its maximum. */
    float resolution_scale = 1.25f;
    /**
     * @brief Ask the compositor to filter the eye images (XR_FB_composition_layer_settings) when available.
     * @details Off by default until it has been compared on the headset with a correctly rendered image.
     */
    bool use_compositor_supersampling = false;
};

/**
 * @brief Platform-agnostic XR session lifecycle.
 *
 * Backends: Null (CI), OpenXR (Android/Windows), CompositorServices (visionOS).
 */
class VNE_XR_API ISession {
   public:
    virtual ~ISession() = default;

    [[nodiscard]] virtual BackendType backendType() const = 0;
    [[nodiscard]] virtual SessionState state() const = 0;

    /** @brief Poll runtime events; return false to exit main loop. */
    virtual bool pollEvents() = 0;

    /** @brief Advance to next frame; fills @p out_frame. */
    virtual bool beginFrame(Frame& out_frame) = 0;

    /** @brief Submit composition layers for @p frame. */
    virtual void endFrame(const Frame& frame, const LayerParams& layers) = 0;

    /** @brief Optional input snapshot after beginFrame. */
    virtual void pollInput(InputState& out_input) { (void)out_input; }

    /**
     * @brief Ask the runtime to end the session gracefully (the application wants to quit).
     * @return True when the backend will make `pollEvents()` return false once the session has stopped, so the
     * caller keeps running the loop until then. False when the loop should simply stop (the default).
     */
    [[nodiscard]] virtual bool requestExit() { return false; }

    /** @brief Run loop with app render session until pollEvents returns false. */
    virtual void run(IRenderSession& app_session);

    /** @brief True when session can render frames (synchronized/visible/focused). */
    [[nodiscard]] virtual bool isSessionRunning() const {
        const auto s = state();
        return s == SessionState::eSynchronized || s == SessionState::eVisible || s == SessionState::eFocused;
    }

    /**
     * @brief Session-owned graphics device used for compositor swapchain import.
     * @details Apps must render with this device. Creating a second device cannot target OpenXR images.
     */
    [[nodiscard]] virtual std::shared_ptr<vne::rhi::IDevice> graphicsDevice() const { return nullptr; }
};

/** @brief Create session for @p config (factory dispatches by backend). */
[[nodiscard]] VNE_XR_API std::unique_ptr<ISession> createSession(const SessionConfig& config);

}  // namespace vne::xr
