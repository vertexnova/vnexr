/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "vertexnova/xr/xr.h"
#include "framework/common/logging_guard.h"

int main() {
    vne::xr::samples::LoggingGuard guard;
    VNE_LOG_INFO << vne::xr::hello();
    VNE_LOG_INFO << "Version: " << vne::xr::getVersion();
    return 0;
}
