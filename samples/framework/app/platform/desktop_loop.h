#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "vertexnova/xr/session.h"

namespace vne::xr::samples {

/** @brief Desktop OpenXR loop (Linux Monado / Windows PC VR). */
class DesktopXrLoop {
   public:
    int run(ISession& session, IRenderSession& render_session);
};

}  // namespace vne::xr::samples
