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
 * @file xr_factory.h
 * @brief XR backend discovery and selection (mirrors vne::rhi::GraphicsFactory).
 */

#include "vertexnova/xr/types.h"

#include <string>
#include <vector>

namespace vne::xr {

/** @brief Stateless XR backend discovery helpers. */
class VNE_XR_API XrFactory {
   public:
    [[nodiscard]] static std::vector<BackendType> getAvailableBackends();
    [[nodiscard]] static bool isBackendAvailable(BackendType backend);
    [[nodiscard]] static BackendType getDefaultBackend();
    [[nodiscard]] static BackendType selectBestBackend(BackendType preferred);
    [[nodiscard]] static std::string backendToString(BackendType backend);
    [[nodiscard]] static std::string getAvailableBackendsString();

   private:
    XrFactory() = delete;
};

}  // namespace vne::xr
