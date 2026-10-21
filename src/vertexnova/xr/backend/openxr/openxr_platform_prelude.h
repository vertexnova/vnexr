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

/**
 * @file openxr_platform_prelude.h
 * @brief Include platform APIs before openxr_platform.h.
 *
 * Khronos openxr_platform.h does not pull in vulkan.h / jni.h / windows.h;
 * those must be visible first when XR_USE_GRAPHICS_API_VULKAN /
 * XR_USE_PLATFORM_ANDROID / XR_USE_PLATFORM_WIN32 are defined.
 */

#include <openxr/openxr.h>

#if defined(VNE_XR_OPENXR_GRAPHICS_VULKAN) || defined(XR_USE_GRAPHICS_API_VULKAN)
#include <vulkan/vulkan.h>
#endif

#if defined(XR_USE_PLATFORM_ANDROID) || defined(__ANDROID__)
#include <jni.h>
#endif

#if defined(XR_USE_PLATFORM_WIN32) || defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <openxr/openxr_platform.h>
