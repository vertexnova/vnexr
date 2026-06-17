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

#include "vertexnova/xr/backend/openxr/openxr_instance_lifetime.h"

namespace vne::xr::OpenXrInstanceLifetime {

namespace {

std::mutex g_mutex;
XrInstance g_instance = XR_NULL_HANDLE;
std::atomic<std::uint32_t> g_user_count{0};

}  // namespace

void registerInstanceUser(XrInstance instance) noexcept {
    std::lock_guard lock(g_mutex);
    if (g_instance == XR_NULL_HANDLE) {
        g_instance = instance;
    }
    ++g_user_count;
}

void unregisterInstanceUser(XrInstance instance) noexcept {
    (void)instance;
    std::lock_guard lock(g_mutex);
    if (g_user_count > 0) {
        --g_user_count;
    }
    if (g_user_count == 0) {
        g_instance = XR_NULL_HANDLE;
    }
}

XrInstance getTrackedInstance() noexcept {
    std::lock_guard lock(g_mutex);
    return g_instance;
}

std::uint32_t getUserCount() noexcept {
    return g_user_count.load();
}

}  // namespace vne::xr::OpenXrInstanceLifetime
