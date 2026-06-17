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
 * @file xr_error.h
 * @brief XR-wide result codes and helpers (parallel to vne::rhi::Result).
 */

#include "vertexnova/xr/export.h"

#include <cstdint>
#include <string_view>

namespace vne::xr {

/** @brief XR result code. Success ≥ 0; errors < 0. */
enum class Result : std::int32_t {
    eSuccess = 0,
    eNotReady = 1,
    eTimeout = 2,
    eSessionNotRunning = 3,

    eErrorOutOfMemory = -1,
    eErrorInitializationFailed = -2,
    eErrorInvalidArgument = -3,
    eErrorNotSupported = -4,
    eErrorBadState = -5,
    eErrorUnknown = -6,
    eErrorOpenXrFailed = -100,
};

[[nodiscard]] constexpr std::string_view toString(Result result) noexcept {
    switch (result) {
        case Result::eSuccess:
            return "eSuccess";
        case Result::eNotReady:
            return "eNotReady";
        case Result::eTimeout:
            return "eTimeout";
        case Result::eSessionNotRunning:
            return "eSessionNotRunning";
        case Result::eErrorOutOfMemory:
            return "eErrorOutOfMemory";
        case Result::eErrorInitializationFailed:
            return "eErrorInitializationFailed";
        case Result::eErrorInvalidArgument:
            return "eErrorInvalidArgument";
        case Result::eErrorNotSupported:
            return "eErrorNotSupported";
        case Result::eErrorBadState:
            return "eErrorBadState";
        case Result::eErrorOpenXrFailed:
            return "eErrorOpenXrFailed";
        case Result::eErrorUnknown:
        default:
            return "eErrorUnknown";
    }
}

[[nodiscard]] constexpr bool isSuccess(Result result) noexcept {
    return result >= Result::eSuccess;
}

[[nodiscard]] constexpr bool isError(Result result) noexcept {
    return result < Result::eSuccess;
}

}  // namespace vne::xr

#define VNE_XR_CHECK(expr)                          \
    do {                                            \
        ::vne::xr::Result _vne_xr_result = (expr);  \
        if (::vne::xr::isError(_vne_xr_result)) {   \
            return _vne_xr_result;                  \
        }                                           \
    } while (0)

#define VNE_XR_CHECK_BOOL(expr)                     \
    do {                                            \
        ::vne::xr::Result _vne_xr_result = (expr);  \
        if (::vne::xr::isError(_vne_xr_result)) {   \
            return false;                           \
        }                                           \
    } while (0)
