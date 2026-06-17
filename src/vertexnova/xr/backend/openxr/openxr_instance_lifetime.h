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

#include <openxr/openxr.h>

#include <atomic>
#include <cstdint>
#include <mutex>

namespace vne::xr {

/** @brief Ref-counted XrInstance lifetime (vulkan_instance_lifetime analogue). */
namespace OpenXrInstanceLifetime {

void registerInstanceUser(XrInstance instance) noexcept;
void unregisterInstanceUser(XrInstance instance) noexcept;
[[nodiscard]] XrInstance getTrackedInstance() noexcept;
[[nodiscard]] std::uint32_t getUserCount() noexcept;

}  // namespace OpenXrInstanceLifetime

}  // namespace vne::xr
