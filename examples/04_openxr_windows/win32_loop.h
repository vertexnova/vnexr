#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   June 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

#include "vertexnova/xr/session.h"

namespace vne::xr::examples {

/** @brief Pump Win32 messages between XR frames. */
class Win32Loop {
   public:
    bool pump_messages();
    void request_quit();
    [[nodiscard]] bool should_quit() const;

   private:
    bool quit_ = false;
};

/** @brief OpenXR render session with Diligent T28-style WinMain integration. */
class Win32OpenXrApp final : public IRenderSession {
   public:
    explicit Win32OpenXrApp(Win32Loop& loop);
    void onSessionReady() override;
    void update(const FrameParams& params, LayerParams& out_layers) override;

   private:
    Win32Loop& loop_;
    std::uint64_t frames_seen_ = 0;
};

}  // namespace vne::xr::examples
