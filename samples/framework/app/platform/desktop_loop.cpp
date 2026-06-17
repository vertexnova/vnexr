/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "app/platform/desktop_loop.h"

namespace vne::xr::samples {

int DesktopXrLoop::run(ISession& session, IRenderSession& render_session) {
    render_session.onSessionReady();

    Frame frame{};
    LayerParams layers{};
    InputState input{};

    while (true) {
        if (!session.pollEvents()) {
            break;
        }
        if (!session.isSessionRunning()) {
            continue;
        }
        if (!session.beginFrame(frame)) {
            continue;
        }
        session.pollInput(input);
        FrameParams params{frame};
        params.input = input;
        render_session.update(params, layers);
        session.endFrame(frame, layers);
        if (layers.request_exit) {
            break;
        }
    }
    return 0;
}

}  // namespace vne::xr::samples
